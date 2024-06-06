#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include "LineParser.h"

int debug = 0;

#define MAX_LINE 2048

void execute(cmdLine *pCmdLine);

int stoierr(char *string)
{
    int val;
    sscanf(string, "%d", &val);
    if (val == 0)
    {
        fprintf(stderr, "Error: invalid number");
        return -1;
    }
    return val;
}

void set_debug(int argc, char **argv)
{
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "-d") == 0)
        {
            debug = 1;
            return;
        }
}

int main(int argc, char **argv)
{
    set_debug(argc, argv);

    char buf[MAX_LINE];
    while (1)
    {
        char cwd[PATH_MAX];
        getcwd(cwd, PATH_MAX);
        printf("CWD: %s/\n", cwd);

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
                perror("Error");
            freeCmdLines(line);
            continue;
        }

        else if (strcmp(line->arguments[0], "alarm") == 0)
        {
            if (line->argCount != 2)
                fprintf(stderr, "Error: invalid number of arguments");
            else
            {
                int pid = stoierr(line->arguments[1]);
                if (pid != -1)
                    kill(pid, SIGCONT);
            }
        }

        else if (strcmp(line->arguments[0], "blast") == 0)
        {
            if (line->argCount != 2)
                fprintf(stderr, "Error: invalid number of arguments");
            else
            {
                int pid = stoierr(line->arguments[1]);
                if (pid != -1)
                    kill(pid, SIGINT);
            }
        }

        else
        {
            execute(line);
            freeCmdLines(line);
            line = NULL;
        }
    }

    return 0;
}

void execute(cmdLine *pCmdLine)
{
    int pid = fork();
    if (pid == -1)
    {
        freeCmdLines(pCmdLine);
        _exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        if (debug)
            fprintf(stderr, "PID: %d\nExecuting command: %s\n", pid, pCmdLine->arguments[0]);

        if (pCmdLine->inputRedirect)
        {
            close(STDIN_FILENO);
            int x = open(pCmdLine->inputRedirect, O_RDONLY | O_CREAT, 0777);
            if (x == -1)
            {
                perror("Error");
                freeCmdLines(pCmdLine);
                _exit(EXIT_FAILURE);
            }
        }

        if (pCmdLine->outputRedirect)
        {
            close(STDOUT_FILENO);
            int x = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT, 0777);
            if (x == -1)
            {
                perror("Error");
                freeCmdLines(pCmdLine);
                _exit(EXIT_FAILURE);
            }
        }

        int val = execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        if (val == -1)
        {
            perror("Error");
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
                perror("Error");
                freeCmdLines(pCmdLine);
                _exit(EXIT_FAILURE);
            }
        }
    }
}
