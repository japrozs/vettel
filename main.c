#include <editline/readline.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <stdbool.h>
#include <stdlib.h>

#define BUF_SIZE 2048
#define NO_KEY_FOUND_STR "vettel:key_not_found"
#define DAEMON_NAME "vetteld"
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MATCH(a, b) strncmp(a, b, strlen(b)) == 0
#define CMP(a, b) strncmp(a, b, MAX(strlen(a), strlen(b))) == 0
#define ALLOC(n) malloc(sizeof(char) * BUF_SIZE * n)

// for displaying errors
#define RED_BOLD "\033[1;31m"
#define BLUE_BOLD "\033[1;34m"
#define GRAY "\033[0;37m"
#define RESET "\033[0m"
#define ERR(...)                           \
	printf("%serror%s: ", RED_BOLD, GRAY); \
	printf(__VA_ARGS__);                   \
	printf("%s\n", RESET);
bool FORMAT = 1;

// fmt("hello, %s", "universe") -> "hello, universe"
char *fmt(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char *result;
	vasprintf(&result, fmt, args);
	va_end(args);
	return result;
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
			ERR("unable to create key-value store at $HOME/.store.vt");
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

bool delete_key(char *key)
{
	FILE *fp = fopen(get_store_path(), "r+");
	if (fp == NULL)
	{
		return false;
	}
	fseek(fp, 0, SEEK_END);
	int len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *contents = malloc(sizeof(char) * (len + 1));
	size_t line_len = 0;
	char *line = NULL;
	while ((getline(&line, &line_len, fp)) != -1)
	{
		char *line_cpy = malloc(sizeof(char) * line_len);
		strcpy(line_cpy, line);
		char *line_key = strtok(line, ":");
		if (!(MATCH(line_key, key)))
		{
			sprintf(contents, "%s%s", contents, line_cpy);
		}
	}
	fclose(fp);
	FILE *writer = fopen(get_store_path(), "w");
	if (writer == NULL)
	{
		return false;
	}
	fprintf(writer, "%s", contents);
	fclose(writer);
	return true;
}

char *get_key(char *key, bool show_error)
{
	size_t len = 0;
	char *line = NULL;
	FILE *fp = fopen(get_store_path(), "r");
	while ((getline(&line, &len, fp)) != -1)
	{
		char *line_key = strtok(line, ":");
		if (CMP(line_key, key))
		{
			// found key
			fclose(fp);
			return strtok(NULL, "\n");
		}
	}
	fclose(fp);
	if (show_error)
	{
		char *str = malloc(sizeof(char) * 512);
		sprintf(str, "%serror%s: no record with the key \"%s\" was found%s", RED_BOLD, GRAY, key, RESET);
		// TODO: throw error that no key was found
		return str;
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
		if (FORMAT == 1)
		{
			printf("%s%d)%s \"%s\"\n", GRAY, idx, RESET, line_key);
		}
		else
		{
			printf("%s\n", line_key);
		}
		idx++;
	}
}

bool set_key(char *key, char *value)
{
	FILE *fp = fopen(get_store_path(), "a");
	if (fp == NULL)
	{
		// TODO: show errors
		ERR("unable to access store at $HOME/.store.vt");
		return false;
	}
	fprintf(fp, "%s:%s\n", key, value);
	fclose(fp);
	return true;
}

// parse the commands e.g. 'set <key> <value>'
void parse_input(const char *line)
{
	if (MATCH(line, "quit") || MATCH(line, "q"))
	{
		exit(EXIT_SUCCESS);
	}
	else if (MATCH(line, "help") || MATCH(line, "h"))
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
			ERR("unable to read the key");
			return;
		}
		else if (arr[2] == NULL)
		{
			// TODO: show error, no value present
			ERR("unable to read the value");
			return;
		}
		if (!(MATCH(get_key(arr[1], false), NO_KEY_FOUND_STR)))
		{
			ERR("a record with key \"%s\" already exists", arr[1]);
			return;
		}
		bool ret = set_key(arr[1], merge(arr, 2));
		if (!ret)
		{
			// TODO: show error
			printf("an error occured!\n");
			return;
		}
		printf("ok\n");
	}
	else if (MATCH(line, "get"))
	{
		char **arr = ALLOC(3);
		break_up(arr, line);
		if (arr[1] == NULL)
		{
			// TODO: show error, no key present
			ERR("unable to read the key");
			return;
		}
		printf("%s\n", get_key(arr[1], true));
	}
	else if (MATCH(line, "list"))
	{
		list_all_keys();
	}
	else if (MATCH(line, "delete") || MATCH(line, "del"))
	{
		char **arr = ALLOC(3);
		break_up(arr, line);
		if (arr[1] == NULL)
		{
			// TODO: show error, no key present
			ERR("unable to read the key");
			return;
		}
		if (MATCH(get_key(arr[1], false), NO_KEY_FOUND_STR))
		{
			ERR("no record with that key exists");
			return;
		}
		bool ret = delete_key(arr[1]);
		if (!ret)
		{
			ERR("unable to delete key for some unknown reason");
			return;
		}
		printf("ok\n");
	}
	else if (MATCH(line, "info") || MATCH(line, "i"))
	{
		// TODO: fix this
		printf("this is the info message\n");
	}
	else
	{
		// TODO: show error, no command matched
		ERR("no command with that name exists");
	}
}

int main(int argc, char **argv)
{
	if (argc >= 2 && strcmp(argv[1], "-c") == 0)
	{
		if (argc == 2)
		{
			ERR("unable to read command to run");
			return EXIT_SUCCESS;
		}
		FORMAT = 0;
		parse_input(argv[2]);
		return EXIT_SUCCESS;
	}

	// start the REPL (read, eval, print , loop)
	while (true)
	{
		const char *line = readline(">>> ");
		if (!line)
			break; // exited
		if (strcmp(line, "") == 0)
			continue;
		add_history(line);
		parse_input(line);
	}
	return EXIT_SUCCESS;
}
