#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include "signal.h"

char success[10] = "\033[1;32m";
char failure[10] = "\033[1;31m";
char SPACE_DELIM[10] = " ";
char MULTI_COMMANDS_DELIM[10] = ";";
char DICTIONARY_DELIM[10] = "=";
char PIPE_DELIM[10] = "|";
char REDIRECT_DELIM[10] = ">";

typedef struct word
{
	char *name;
	char *meaning;
} Word;

typedef struct dictionary
{
	int length;
	Word *list[100];
} Dictionary;

void print_cwd(char *statusColor)
{
	char username[20];
	char cwd[100];
	gethostname(username, sizeof(username));
	getcwd(cwd, sizeof(cwd));
	printf("\033[1;96m\e[1m%s:\033[1;93m%s ", username, cwd);
	printf("%s$", statusColor);
	printf("\033[1;0m");
}

void handle_interrupt()
{
	exit(0);
}

int parse_command(char *str, char **multiple_commands, char *delimter)
{
	int idx = 0;
	char *pch = strtok(str, delimter);
	while (pch != NULL)
	{
		multiple_commands[idx] = malloc(100 * sizeof(char));
		strcpy(multiple_commands[idx], pch);
		idx++;
		pch = strtok(NULL, delimter);
	}
	multiple_commands[idx] = NULL;
	return idx - 1;
}

void execute_single_command(char **parameters, char **statusColor, char *f)
{
	if (strcmp(parameters[0], "cd") == 0)
	{
		chdir(parameters[1]);
	}
	else
	{
		int status = execvp(parameters[0], parameters);
		if (status != 0)
		{
			printf("csh: command not found: %s\n", parameters[0]);
			*statusColor = f;
		}
	}
}

Dictionary *add_alias(char **parameters, Dictionary *alias_list, int length)
{
	char **data = malloc(sizeof(char *) * 10);
	int idx = 0;
	char str[100];
	strcpy(str, "");
	for (int i = 1; i < length; i++)
	{
		strcat(str, parameters[i]);
		strcat(str, " ");
	}
	idx = parse_command(str, data, DICTIONARY_DELIM);
	if (idx)
	{
		Word *new_alias = malloc(sizeof(Word));
		new_alias->name = malloc(sizeof(char) * 100);
		new_alias->meaning = malloc(sizeof(char) * 100);
		strcpy(new_alias->name, data[0]);
		strcpy(new_alias->meaning, data[1]);
		alias_list->list[alias_list->length] = new_alias;
		alias_list->length++;
	}
	return alias_list;
}
char **replace_parameter_aliased(char **parameters, Dictionary *alias_list, int length)
{
	int has_alias = 0;
	for (int i = 0; i < alias_list->length; i++)
	{
		if (strcmp(alias_list->list[i]->name, parameters[0]) == 0)
		{
			has_alias = 1;
			memset(parameters[0], 0, 10);
			strcpy(parameters[0], alias_list->list[i]->meaning);
		}
	}
	if (has_alias)
	{
		char *str = malloc(sizeof(char) * 100);
		memset(str, 0, 100);
		for (int i = 0; i < length; i++)
		{
			strcat(str, parameters[i]);
			strcat(str, " ");
		}
		char **new_parameter = malloc(sizeof(char *) * 10);
		char delim[10] = " ";
		int length = parse_command(str, new_parameter, delim);
		parameters = new_parameter;
	}
	return parameters;
}
char **replace_variable_values(char **parameters, Dictionary *variable_list)
{
	int idx = 0;
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < variable_list->length; j++)
		{
			if (strcmp(parameters[i], variable_list->list[j]->name) == 0)
			{
				memset(parameters[i], 0, 10);
				strcpy(parameters[i], variable_list->list[j]->meaning);
			}
		}
	}
	return parameters;
}
void print_alias(Dictionary *alias_list)
{
	for (int i = 0; i < alias_list->length; i++)
	{
		printf("%s=%s\n", alias_list->list[i]->name, alias_list->list[i]->meaning);
	}
}
char *execute(char *str, Dictionary *alias_list, Dictionary *variable_list, char *statusColor)
{
	char **pipedArgs = malloc(10 * sizeof(char *));
	char **multiple_commands = malloc(10 * sizeof(char *));
	char **redirected_commands = malloc(10 * sizeof(char *));
	char buffer[10];
	int is_piped = parse_command(str, pipedArgs, PIPE_DELIM);
	int is_multi_commands = parse_command(str, multiple_commands, MULTI_COMMANDS_DELIM);
	int is_redirected = parse_command(str, redirected_commands, REDIRECT_DELIM);
	if (is_multi_commands)
	{
		for (int i = 0; i < is_multi_commands + 1; i++)
		{
			int pid = fork();
			if (pid == 0)
			{
				char **parameters = malloc(10 * sizeof(char *));
				char **new_parameters = malloc(10 * sizeof(char *));
				int length = parse_command(multiple_commands[i], parameters, SPACE_DELIM) + 1;
				new_parameters = replace_parameter_aliased(parameters, alias_list, length);
				char **new_vars = malloc(10 * sizeof(char *));
				new_vars = replace_variable_values(new_parameters, variable_list);
				free(parameters);
				execute_single_command(new_vars, &statusColor, failure);
			}
			else
			{
				statusColor = success;
				wait(&pid);
			}
		}
	}
	else if (is_redirected + 1 && redirected_commands[1] != NULL)
	{
		int pid;
		char buf[100];
		char space_delim[10] = " ";
		char **parameters = malloc(10 * sizeof(char *));
		int length = parse_command(redirected_commands[0], parameters, SPACE_DELIM) + 1;
		int pipefds[2];
		pipe(pipefds);
		pid = fork();
		if (pid == 0)
		{
			close(pipefds[0]);
			dup2(pipefds[1], STDOUT_FILENO);
			if (execvp(parameters[0], parameters) < 0)
			{
				printf("csh: command not found: %s\n", parameters[0]);
				statusColor = failure;
			}
		}
		else
		{
			char buffer[100];
			wait(&pid);
			close(pipefds[1]);
			FILE *a = fopen(redirected_commands[1], "w");
			while (read(pipefds[0], buffer, 100))
			{
				fprintf(a, "%s", buffer);
				memset(buffer, 0, 100);
			}
			fclose(a);
			close(pipefds[0]);
			statusColor = success;
		}
	}
	else if (is_piped)
	{
		int pid;
		char buf[100];
		memset(buf, 0, 100);
		pid = fork();
		if (pid == 0)
		{
			int pipefds[2];
			pipe(pipefds);
			pid = fork();
			if (pid == 0)
			{
				char **parameters = malloc(10 * sizeof(char *));
				int length = parse_command(pipedArgs[0], parameters, SPACE_DELIM);
				char **new_parameters = malloc(10 * sizeof(char *));
				new_parameters = replace_parameter_aliased(parameters, alias_list, length);
				char **new_vars = malloc(10 * sizeof(char *));
				new_vars = replace_variable_values(new_parameters, variable_list);
				close(pipefds[0]);
				dup2(pipefds[1], STDOUT_FILENO);
				execute_single_command(new_vars, &statusColor, failure);
			}
			char **parameters = malloc(10 * sizeof(char *));
			int length = parse_command(pipedArgs[1], parameters, SPACE_DELIM);
			char **new_parameters = malloc(10 * sizeof(char *));
			printf("%s", parameters[0]);
			new_parameters = replace_parameter_aliased(parameters, alias_list, length);
			close(pipefds[1]);
			dup2(pipefds[0], STDIN_FILENO);
			execute_single_command(new_parameters, &statusColor, failure);
		}
		else
		{
			statusColor = success;
			wait(&pid);
		}
	}
	else
	{
		char **parameters = malloc(10 * sizeof(char *));
		int length = parse_command(str, parameters, SPACE_DELIM) + 1;
		if (strcmp(parameters[0], "alias") == 0)
		{
			if (length == 1)
			{
				print_alias(alias_list);
				return success;
			}
			alias_list = add_alias(parameters, alias_list, length);
			return success;
		}
		char **variables = malloc(10 * sizeof(char *));
		int is_variable = parse_command(str, variables, DICTIONARY_DELIM);
		if (is_variable)
		{
			Word *new_variable = malloc(sizeof(Word));
			new_variable->name = malloc(sizeof(char) * 10);
			new_variable->meaning = malloc(sizeof(char) * 10);
			strcpy(new_variable->name, "$");
			strcat(new_variable->name, variables[0]);
			strcpy(new_variable->meaning, variables[1]);
			variable_list->list[variable_list->length] = new_variable;
			variable_list->length++;
			return success;
		}
		char **new_parameters = malloc(10 * sizeof(char *));
		new_parameters = replace_parameter_aliased(parameters, alias_list, length);
		char **new_vars = malloc(10 * sizeof(char *));
		new_vars = replace_variable_values(new_parameters, variable_list);
		int pid = fork();
		if (pid == 0)
		{
			signal(SIGINT, handle_interrupt);
			execute_single_command(new_parameters, &statusColor, failure);
		}
		else
		{
			statusColor = success;
			wait(&pid);
		}
	}
	return statusColor;
}

int main()
{
	Dictionary *alias_list = malloc(sizeof(Dictionary));
	Dictionary *variable_list = malloc(sizeof(Dictionary));
	variable_list->length = 0;
	alias_list->length = 0;
	char buf[1000];
	char str[1000];
	signal(SIGINT, SIG_IGN);
	FILE *cshrc = fopen("test", "r");
	char *statusColor = success;
	if (cshrc != NULL)
	{
		while (fscanf(cshrc, "  %[^\n]", buf) == 1)
		{
			statusColor = execute(buf, alias_list, variable_list, statusColor);
			memset(buf, 0, 1000);
		}
		fclose(cshrc);
	}
	while (1)
	{
		char *statusColor = success;
		print_cwd(statusColor);
		scanf(" %[^\n]", str);
		if (strcmp(str, "exit") == 0)
		{
			return 0;
		}
		statusColor = execute(str, alias_list, variable_list, statusColor);
	}
}