#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUFSIZE 128

char prompt[] = "rsh%: "; // user prompt

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

int main() {
	char line[BUFSIZE];
	char *args[16];
	char *path = getenv("PATH");
	int pid, status;

	while (Prompt(line, BUFSIZE) && strcmp("exit", line)) {
		ParseLine(line, args);

		if (fork()) { // parent
			pid = wait(&status);
			printf("exit status: %d\n", status);
		}
		else { // child
			ChangeImage(path, args);
			printf("Couldn't find %s\n", args[0]);
			exit(1);
		}
	}
}
