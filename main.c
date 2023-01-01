#include <editline/readline.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <stdbool.h>
#include <stdlib.h>

// for the server
#include <arpa/inet.h>
#include <sys/socket.h>

// for the daemon
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE 2048
#define NO_KEY_FOUND_STR "vettel:key_not_found"
#define DAEMON_NAME "vetteld"
#define MATCH(a, b) strncmp(a, b, strlen(b)) == 0
#define ALLOC(n) malloc(sizeof(char) * BUF_SIZE * n)

// server size
#define BUFSIZE 4096
#define PORT 7072
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 100
#define SERVER_NAME "vetteld@0x6a6170726f7a/0.0.1"

// daemon flags
#define VT_NO_CHDIR 01			/* Don't chdir ("/") */
#define VT_NO_CLOSE_FILES 02	/* Don't close all open files */
#define VT_NO_REOPEN_STD_FDS 04 /* Don't reopen stdin, stdout, and stderr to /dev/null */
#define VT_NO_UMASK0 010		/* Don't do a umask(0) */
#define VT_MAX_CLOSE 8192		/* Max file descriptors to close if \
								   sysconf(_SC_OPEN_MAX) is indeterminate */

void register_daemon()
{
	pid_t pid;
	pid = fork();

	/* An error occurred */
	if (pid < 0)
	{
		printf("here 1\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	pid = fork();
	if (pid < 0)
	{
		// TODO: couldn't create daemon
		printf("here 3\n");
		exit(EXIT_FAILURE);
	}

	/* Close all open file descriptors */
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
	{
		close(x);
	}
}

int check(int exp, const char *msg)
{
	if (exp == SOCKETERROR)
	{
		perror(msg);
		exit(1);
	}

	return exp;
}

void start_server()
{
	int server_socket, client_socket, addr_size;
	struct sockaddr_in server_addr, client_addr;

	check((server_socket = socket(AF_INET, SOCK_STREAM, 0)),
		  "Failed to create socket");

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	check(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)),
		  "Bind failed!");

	check(listen(server_socket, SERVER_BACKLOG),
		  "Listen failed!");

	while (true)
	{
		printf("waiting for connections .... \n");
		// wait, and eventually accept an incoming connection
		check(client_socket = (accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&addr_size)),
			  "accept failed!");

		char buf[5000];
		recv(client_socket, buf, 5000, 0);

		char *route = strtok(buf, " ");
		route = strtok(NULL, " ");
		if (route[strlen(route) - 1] == '/' && strlen(route) != 1)
		{
			route[strlen(route) - 1] = '\0';
		}

		printf("route accessed :: %s\n", route);
		char res[1 << 17];
		sprintf(res,
				"HTTP/1.1 200 OK\n"
				"Server: %s\n"
				"Content-Type: application/json\n"
				"\n"
				"{\n"
				"\t\"command\" : \"whatup\",\n"
				"\t\"output\" : \"hi there\"\n"
				"}",
				SERVER_NAME);
		write(client_socket, res, strlen(res));
		close(client_socket);
	}
}

int start_daemon(int flags)
{
	int maxfd, fd;

	switch (fork()) // become background process
	{
	case -1:
		return -1;
	case 0:
		break; // child falls through
	default:
		_exit(EXIT_SUCCESS); // parent terminates
	}

	if (setsid() == -1) // become leader of new session
		return -1;

	switch (fork())
	{
	case -1:
		return -1;
	case 0:
		break; // child breaks out of case
	default:
		_exit(EXIT_SUCCESS); // parent process will exit
	}

	if (!(flags & VT_NO_UMASK0))
		umask(0); // clear file creation mode mask

	if (!(flags & VT_NO_CHDIR))
		chdir("/"); // change to root directory

	if (!(flags & VT_NO_CLOSE_FILES)) // close all open files
	{
		maxfd = sysconf(_SC_OPEN_MAX);
		if (maxfd == -1)
			maxfd = VT_MAX_CLOSE; // if we don't know then guess
		for (fd = 0; fd < maxfd; fd++)
			close(fd);
	}

	if (!(flags & VT_NO_REOPEN_STD_FDS))
	{
		close(STDIN_FILENO);

		fd = open("/dev/null", O_RDWR);
		if (fd != STDIN_FILENO)
			return -1;
		if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
			return -2;
		if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
			return -3;
	}

	return 0;
}

char *get_store_path()
{
	char *path = malloc(sizeof(char) * 256);
	sprintf(path, "%s/.store.vt", getenv("HOME"));
	FILE *fp = fopen(path, "r");
	if (fp == NULL)
	{
		FILE *w = fopen(path, "w");
		if (w == NULL)
		{
			// TODO: provide a better error message
			printf("couldn't initialize store at \"%s\"\n", path);
			exit(0);
		}
		fclose(w);
	}
	fclose(fp);
	return path;
}

void break_up(char **arr, const char *line)
{
	char *word = strtok((char *)line, " ");
	uint i = 0;

	while (word != NULL)
	{
		arr[i] = word;
		i++;
		word = strtok(NULL, " ");
	}
	arr[i] = NULL;
}

char *merge(char **arr, int idx)
{
	if (arr[idx + 1] == NULL)
	{
		return arr[idx];
	}
	char *str = malloc(sizeof(char) * BUF_SIZE);
	str = arr[idx];
	for (int i = idx + 1; arr[i] != NULL; i++)
	{
		sprintf(str, "%s %s", str, arr[i]);
	}
	return str;
}

char *get_key(char *key, bool show_error)
{
	size_t len = 0;
	char *line = NULL;
	FILE *fp = fopen(get_store_path(), "r");
	while ((getline(&line, &len, fp)) != -1)
	{
		char *line_key = strtok(line, ":");
		if (MATCH(line_key, key))
		{
			// found key
			fclose(fp);
			return strtok(NULL, "\n");
		}
	}
	fclose(fp);
	if (show_error)
	{
		// TODO: throw error that no key was found
		return "no key was found!";
	}
	else
	{
		return NO_KEY_FOUND_STR;
	}
}

void list_all_keys()
{
	size_t len = 0;
	char *line = NULL;
	FILE *fp = fopen(get_store_path(), "r");
	uint idx = 0;
	while ((getline(&line, &len, fp)) != -1)
	{
		char *line_key = strtok(line, ":");
		printf("%d) \"%s\"\n", idx, line_key);
		idx++;
	}
}

bool set_key(char *key, char *value)
{
	FILE *fp = fopen(get_store_path(), "a");
	if (fp == NULL)
	{
		// TODO: show errors
		return false;
	}
	fprintf(fp, "%s:%s\n", key, value);
	fclose(fp);
	return true;
}

void parse_input(const char *line)
{
	if (MATCH(line, "quit") || MATCH(line, "q"))
	{
		exit(EXIT_SUCCESS);
	}
	else if (MATCH(line, "help"))
	{
		// TODO: fix this
		printf("this is the help message\n");
	}
	else if (MATCH(line, "set"))
	{
		char **arr = ALLOC(100);
		break_up(arr, line);
		if (arr[1] == NULL)
		{
			// TODO: show error, no key present
			printf("can't access key\n");
			return;
		}
		else if (arr[2] == NULL)
		{
			// TODO: show error, no value present
			printf("can't access value\n");
			return;
		}
		if (!(MATCH(get_key(arr[1], false), NO_KEY_FOUND_STR)))
		{
			printf("a record with that name already exists\n");
			printf("\"%s\" : \"%s\"\n", arr[1], get_key(arr[1], true));
			return;
		}
		bool ret = set_key(arr[1], merge(arr, 2));
		if (!ret)
		{
			// TODO: show error
			printf("an error occured!\n");
			return;
		}
	}
	else if (MATCH(line, "get"))
	{
		char **arr = ALLOC(3);
		break_up(arr, line);
		if (arr[1] == NULL)
		{
			// TODO: show error, no key present
			printf("can't access key\n");
		}
		printf("%s\n", get_key(arr[1], true));
	}
	else if (MATCH(line, "list"))
	{
		list_all_keys();
	}
	else
	{
		// TODO: show error, no command matched
		printf("no command matched!\n");
	}
}

int main(void)
{
	int pid = fork();
	if (pid >= 0 && pid == 0)
	{
		printf("in the child process");
		int ret = start_daemon(pid);
		if (ret)
		{
			syslog(LOG_USER | LOG_ERR, "error starting the daemon");
			closelog();
			return EXIT_FAILURE;
		}

		start_server();

		return EXIT_SUCCESS;
	}
	while (true)
	{
		const char *line = readline(">>> ");
		if (!line)
			break; // exited
		add_history(line);
		parse_input(line);
	}
	return EXIT_SUCCESS;
}
