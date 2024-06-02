#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <string.h>

#include "LineParser.h"

int debug = 0;

#define MAX_LINE 2048

void execute(cmdLine *pCmdLine);

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "-D") == 0)
        debug = 1;

    char buf[MAX_LINE] = {0};
    while (1)
    {
        printf("Current working directory: %s\n", getcwd(NULL, PATH_MAX)); // is this correct?

        printf("Enter command: ");
        fgets(buf, MAX_LINE, stdin);
        cmdLine *line = parseCmdLines(buf);

        if (strcmp(line->arguments[0], "quit") == 0)
        {
            freeCmdLines(line);
            exit(EXIT_SUCCESS);
        }

        execute(line);
        freeCmdLines(line);
        line = NULL;
    }

    return 0;
}

void execute(cmdLine *pCmdLine)
{
    int pid = fork(); // maybe change to pid_t?
    if (pid == -1)
    {
        freeCmdLines(pCmdLine);
        exit(EXIT_FAILURE);
    }
    int val = execvp(pCmdLine->arguments[0], pCmdLine->arguments);
    if (val == -1)
    {
        perror("Execution failed\n");
        freeCmdLines(pCmdLine);
        exit(EXIT_FAILURE);
    }
}
