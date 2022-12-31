#include <editline/readline.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define BUF_SIZE 2048
#define NO_KEY_FOUND_STR "vettel:key_not_found"
#define MATCH(a, b) strncmp(a, b, strlen(b)) == 0
#define ALLOC(n) malloc(sizeof(char) * BUF_SIZE * n)

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
