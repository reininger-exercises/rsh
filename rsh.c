/* Reid Reininger
 * charles.reininger@wsu.edu
 * 
 * Reid's sh (rsh). This is a shell program to execute commands on a linux
 * system. Supports pipes and IO redirection.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>

#define BUFSIZE 128

// IO redirection types
typedef enum redirection {
	INPUT, OUTPUT, APPEND, NONE
} Redirection;

// Table of redirection symbols in commands
const char *redirectionTable[] = {"<", ">", ">>", NULL};

// Command prompt
char prompt[] = "\x1B[34mrsh%\x1B[0m "; 

// Prompts the user for a command. Returns pointer to buf or NULL if EOF.
char *Prompt(char *buf, int size)
{
	printf("%s", prompt);
	char *line = fgets(buf, size, stdin);
	buf[strlen(buf)-1] = '\0'; // remove newline
	return line;
}

// Tokenize line into null terminated list placed in args.
void ParseLine(char *line, char* args[])
{
	char *token = strtok(line, " ");
	args[0] = token;
	for (int i=1; token; i++) {
		token = strtok(NULL, " ");
		args[i] = token;
	}
}

// Change the current working directory.
int ChangeDir(char *args[])
{
	char *path = args[1] ? args[1] : getenv("HOME");
	int status = chdir(path);
	if (status) {
		printf("Could not find %s", path);
	}
	return status;
}

// Change execution image. Command should be first element of args. Searches all directories in path
// delimited by ":" for command. Return -1 if command could not be found.
int ChangeImage(char *path, char *args[])
{
	char command[128];
	char *token = strtok(path, ":");
	while (token) {
		strcpy(command, token);
		strcat(command, "/");
		strcat(command, args[0]);
		execv(command, args);
		token = strtok(NULL, ":");
	}

	execv(args[0], args); // absolute path
	return -1;
}


// Check command for redirection. Redirection type is returned. Redirection
// symbol is replaced with NULL and path to file is placed in path.
Redirection CheckRedirection(char *args[], int *path)
{
	for (int i=0; args[i]; i++) {
		for (int j=0; redirectionTable[j]; j++) {
			if (!strcmp(args[i], redirectionTable[j])) {
				args[i] = NULL;
				*path = i+1;
				return j;
			}
		}
	}
	return NONE;
}

// Redirect input to path. Return -1 if could not open file.
int RedirectIn(char *path) {
	close(0);
	return open(path, O_RDONLY);
}

// Redirect output to path for writing or appending. Return -1 if could not
// open file.
int RedirectOut(char *path, bool append) {
	int mode = O_CREAT|O_WRONLY;
	if (append) {
		mode |= O_APPEND;
	}
	close(1);
	return open(path, mode, 0644);
}

// Split args so args points to head command and returns pointer to tail
// command. Commands are delimited by "|".
char **Split(char *args[])
{
	for (int i=0; args[i]; i++) {
		if (!strcmp(args[i], "|")) {
			args[i] = NULL;
			return &args[i+1];
		}
	}
	return NULL;
}

// Perform IO redirections.
void DoRedirection(char *args[])
{
	int tail;
	switch(CheckRedirection(args, &tail)) {
		case INPUT: RedirectIn(args[tail]); break;
		case OUTPUT: RedirectOut(args[tail], false); break;
		case APPEND: RedirectOut(args[tail], true); break;
		default: break;
	}
}

// Create a pipe to replace stdin and stdout. Returns 1 for the writer process, 0 for the reader.
int CreatePipe()
{
	int pd[2];
	pipe(pd);

	if (fork()) { // writer
		close(pd[0]);
		dup2(pd[1], 1);
		close(pd[1]);
		return 1;
	}
	else { // reader
		close(pd[1]);
		dup2(pd[0], 0);
		close(pd[0]);
		return 0;
	}
}

int DoCommand(char *args[])
{
	char **head = args;
	char **tail = Split(args);

	if (!tail) { // base case, execute single command
		DoRedirection(head);
		ChangeImage(getenv("PATH"), head); // only returns on error
		printf("Couldn't find %s\n", head[0]);
		exit(1);
	}

	// recursive case
	if (CreatePipe()) { // writer
		DoCommand(head);
	}
	else { // reader
		DoCommand(tail);
	}
	
}

int main() {
	char line[BUFSIZE];
	char *args[16];
	char *str;
	int status;

	while (true) {
		str = Prompt(line, BUFSIZE);
		ParseLine(line, args);

		// change directory
		if (!strcmp(args[0], "cd")) {
			ChangeDir(args);
		}

		// exit
		else if (!str || !strcmp(args[0], "exit")) {
			if (!str) putchar('\n');
			exit(0);
		}

		// change image
		else {
			if (fork()) { // parent
				wait(&status);
				printf("exit status: %d\n", status);
			}
			else { // child
				DoCommand(args);
			}
		}
	}
}

