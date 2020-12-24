/* Reid Reininger
 * charles.reininger@wsu.edu
 * 
 * Reid's sh (rsh). This is a shell program to execute commands on a linux
 * system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include "rsh.h"

#define BUFSIZE 128

char prompt[] = "\x1B[34mrsh%\x1B[0m "; // user prompt

// Prompts the user for a command. Returns NULL if EOF without any chars.
char *Prompt(char *buf, int size)
{
	char *str;

	// display prompt
	printf("%s", prompt);

	// read line
	str = fgets(buf, size, stdin);
	buf[strlen(buf)-1] = 0;
	return str;
}

// parse line into command and args. args is null terminated.
void ParseLine(char *line, char* args[])
{
	char *token = strtok(line, " ");
	args[0] = token;
	for (int i=1; token; i++) {
		token = strtok(NULL, " ");
		args[i] = token;
	}
}

// Change executtion image with args. Searches all directories in path for
// command.
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

int ChangeDir(char *args[])
{
	char *path;
	int status;
	path = args[1] ? args[1] : getenv("HOME");
	status = chdir(path);
	if (status) {
		printf("Could not find %s", path);
	}
	return status;
}

// Checks for redirection. Replaces redirection arg with NULL and places index
// of file to redirect to in arg. Returns redirection type.
Redirection CheckRedirection(char *args[], int *arg)
{
	for (int i=0; args[i]; i++) {
		for (int j=0; redirectionTable[j]; j++) {
			if (!strcmp(args[i], redirectionTable[j])) {
				args[i] = NULL;
				*arg = i+1;
				return j;
			}
		}
	}
	return NONE;
}

// Redirect input to path.
int RedirectIn(char *path) {
	close(0);
	return open(path, O_RDONLY);
}

// Redirect output to path for writing or appending.
int RedirectOut(char *path, bool append) {
	int mode = O_CREAT|O_WRONLY;
	if (append) {
		mode |= O_APPEND;
	}
	close(1);
	return open(path, mode, 0644);
}

// perfrom any redirections and fromat args as necessary
void DoRedirects(char *args[])
{
	int index;
	switch(CheckRedirection(args, &index)) {
		case INPUT: RedirectIn(args[index]); break;
		case OUTPUT: RedirectOut(args[index], false); break;
		case APPEND: RedirectOut(args[index], true); break;
		default: break;
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
			putchar('\n');
			exit(0);
		}

		// change image
		else {
			if (fork()) { // parent
				wait(&status);
				printf("exit status: %d\n", status);
			}
			else { // child
				DoRedirects(args);
				ChangeImage(getenv("PATH"), args);
				printf("Couldn't find %s\n", args[0]);
				exit(1);
			}
		}
	}
}
