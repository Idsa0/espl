#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include "LineParser.h"

int debug = 0;

#define MAX_LINE 2048

void execute(cmdLine *pCmdLine);

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "-d") == 0)
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

        if (strcmp(line->arguments[0], "cd") == 0)
        {
            if (chdir(line->arguments[1]) == -1)
                perror("Error: ");
            freeCmdLines(line);
            continue;
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
        _exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        if (debug)
            fprintf(stderr, "PID: %d\nExecuting command: %s\n", pid, pCmdLine->arguments[0]);

        int val = execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        if (val == -1)
        {
            perror("Error: ");
            freeCmdLines(pCmdLine);
            _exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (pCmdLine->blocking)
        {
            int status;
            if (waitpid(pid, &status, 0) == -1)
            {
                perror("Error: ");
                freeCmdLines(pCmdLine);
                _exit(EXIT_FAILURE); // should I exit here?
            }
        }
    }
}
