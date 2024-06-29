#include <linux/limits.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "LineParser.h"

// TODO: need to add created child processes to proclist via addProcess func

//---------------------------------process list---------------------------------------------------------
#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0

typedef struct process
{
    cmdLine *cmd;         /* the parsed command line*/
    pid_t pid;            /* the process id that is running the command*/
    int status;           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next; /* next process in chain */
} process;

void addProcess(process **process_list, cmdLine *cmd, pid_t pid);
void printProcessList(process **process_list);
void performProcs();

void freeProcessList(process *process_list);
void updateProcessList(process **process_list);
void updateProcessStatus(process *process_list, int pid, int status);
void removeAllDeadProcesses(process **curr_process_list);

void printArr(cmdLine *pCmdLine);

void cleanEnv();

// global variable
process *process_list = NULL;

//---------------------------------------history---------------------------------------------------------
#define HISTLEN 20
#define MAX_BUF 200
int oldest = 0, newest = 0;
int HistorySize = 0;
char history[HISTLEN][MAX_BUF];

void addToHistory(char *command);
void performHistory();
void performLastCommand();
void performTheNthCommand(cmdLine *currLine);
char *getCommandFromHistory(int index);

//---------------------------------process control---------------------------------------------------------
int checkString(char *src, char *dst, int dstSize);
int checkFlag(int argc, char **argv, char *flag);

void execute(cmdLine *pCmdLine);

void performCD(cmdLine *pCmdLine);

void performAlarm(cmdLine *pCmdLine);
void performBlast(cmdLine *pCmdLine);
void performSleep(cmdLine *pCmdLine);

// global variables
int isInDebug = 0;

int main(int argc, char **argv)
{
    isInDebug = checkFlag(argc, argv, "-d");

    while (1)
    {
        char buf[PATH_MAX];
        if (!getcwd(buf, PATH_MAX))
            exit(EXIT_FAILURE);
        printf("%s\n", buf);

        int size = 2048;
        char input[size];
        if (!fgets(input, size, stdin))
            exit(EXIT_FAILURE);

        if (strcmp(input, "quit\n") == 0)
        {
            cleanEnv();
            break;
        }

        cmdLine *currLine = parseCmdLines(input);
        if (!currLine)
            exit(EXIT_FAILURE);

        if (strcmp((*currLine).arguments[0], "cd") == 0)
            performCD(currLine);
        else if (strcmp((*currLine).arguments[0], "alarm") == 0)
            performAlarm(currLine);
        else if (strcmp((*currLine).arguments[0], "blast") == 0)
            performBlast(currLine);
        else if (strcmp((*currLine).arguments[0], "sleep") == 0)
            performSleep(currLine);
        else if (strcmp((*currLine).arguments[0], "procs") == 0)
            performProcs();
        else if (strcmp((*currLine).arguments[0], "history") == 0)
            performHistory();
        else if (strcmp((*currLine).arguments[0], "!!") == 0)
            performLastCommand();
        else if (currLine->arguments[0][0] == '!')
            performTheNthCommand(currLine);
        else
            execute(currLine);

        // add to the history
        addToHistory(input);
    }

    exit(0);
}

void execute(cmdLine *pCmdLine)
{
    // create and open a pipe
    int buf[2]; // buf[0] is for reading, buf[1] is for writing
    if (pipe(buf) == -1)
    {
        perror("pipe error");
        exit(1);
    }

    //-----------------fist child----------------------------

    int childPID = fork();

    if (childPID == 0)
    {
        fprintf(stderr, "child #1 was created. PID: %d\tCommand: %s\n", getpid(), pCmdLine->arguments[0]);

        // debug mode
        if (isInDebug)
            fprintf(stderr, "PID: %d\tCommand: %s\n", getpid(), pCmdLine->arguments[0]);

        // printf("input: %s\noutput: %s",pCmdLine->inputRedirect, pCmdLine->outputRedirect);
        // printf("output: %s\n",pCmdLine->outputRedirect);

        // Input redirection
        if (pCmdLine->inputRedirect)
        {
            printf("changing input stream on first child\n");
            close(STDIN_FILENO);
            open(pCmdLine->inputRedirect, O_RDONLY, 0777);
        }

        // Output redirection
        if (pCmdLine->next)
        {
            printf("changing output stream on first child\n");
            close(STDOUT_FILENO);
            // open(pCmdLine->outputRedirect, O_RDWR	 | O_CREAT, 0777);
            dup(buf[1]);
            close(buf[1]);
        }
        else if (pCmdLine->outputRedirect)
        {
            close(STDOUT_FILENO);
            open(pCmdLine->outputRedirect, O_RDWR | O_CREAT, 0777);
        }

        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("execv failed on first child! ");
        _exit(1);
    }

    // closing the write-end of the pipe
    fprintf(stderr, "parent_process>closing the write end of the pipe…\n");
    close(buf[1]);

    addProcess(&process_list, pCmdLine, childPID);

    if (pCmdLine->blocking)
    {
        printf("waiting....\n");
        waitpid(childPID, &(*process_list).status, 0);
    }

    //-----------------second child----------------------------

    if (pCmdLine->next)
    {
        pCmdLine = pCmdLine->next;

        childPID = fork();
        if (childPID == 0)
        {
            fprintf(stderr, "child #2 was created. PID: %d\tCommand: %s\n", getpid(), pCmdLine->arguments[0]);

            // Input redirection
            printf("changing input stream on second child\n");
            close(STDIN_FILENO);
            dup(buf[0]);
            close(buf[0]);

            // Output redirection
            if (pCmdLine->outputRedirect)
            {
                printf("changing output stream on second child\n");
                close(STDOUT_FILENO);
                open(pCmdLine->outputRedirect, O_RDWR | O_CREAT, 0777);
            }

            execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            perror("execv failed on second child! ");
            _exit(1);
        }

        // closing the read-end of the pipe
        fprintf(stderr, "parent_process>closing the read end of the pipe… \n");
        close(buf[0]);

        addProcess(&process_list, pCmdLine, childPID);

        if (pCmdLine->blocking)
        {
            printf("waiting....\n");
            waitpid(childPID, &(*process_list).status, 0);
        }
    }

    // exiting
    fprintf(stderr, "\nparent_process>exiting… \n");
    return;
}

// return 1 if the debug flag was provided, 0 otherwise.
int checkFlag(int argc, char **argv, char *flag)
{
    for (int i = 1; i < argc; ++i)
    {
        // check for a debug flag
        if (strcmp(argv[i], flag) == 0)
        {
            printf("%s is on\n", flag);
            return 1;
        }
    }

    printf("%s is off\n", flag);
    return 0;
}

// return 1 if src's initial string is equls to dst (assumes src is always larger than dst)
int checkString(char *src, char *dst, int dstSize)
{
    for (int i = 0; i < dstSize; ++i)
        if (src[i] != dst[i])
            return 1;

    return 0;
}

void performCD(cmdLine *pCmdLine)
{
    // printf("doing CD command\n");
    // printf("1:%s 2:%s 3:%s \n", pCmdLine->arguments[0], pCmdLine->arguments[1], pCmdLine->arguments[2]);

    if (chdir(pCmdLine->arguments[1]))
    {
        fprintf(stderr, "cd command failed with path %s with error ", pCmdLine->arguments[1]);
        perror("");
    }
}

void performAlarm(cmdLine *pCmdLine)
{
    int result = kill(atoi(pCmdLine->arguments[1]), SIGCONT);
    if (result == 0)
        printf("Alarm succeeded.\n");
    else
        printf("Alarm failed.\n");
}

void performBlast(cmdLine *pCmdLine)
{
    int result = kill(atoi(pCmdLine->arguments[1]), SIGKILL);
    if (result == 0)
        printf("Blast succeeded.\n");
    else
        printf("Blast failed.\n");
}

void performSleep(cmdLine *pCmdLine)
{
    int result = kill(atoi(pCmdLine->arguments[1]), SIGTSTP);
    if (result == 0)
        printf("Sleep succeeded.\n");
    else
        printf("Sleep failed.\n");
}

//-------------process list functions

void addProcess(process **curr_process_list, cmdLine *cmd, pid_t pid)
{
    // create a new instance of process
    process *my_struct;
    my_struct = (process *)calloc(1, sizeof(*my_struct));

    (my_struct)->cmd = cmd;
    (my_struct)->pid = pid;
    (my_struct)->status = RUNNING;

    my_struct->next = *curr_process_list;
    *curr_process_list = (my_struct);
}

void printProcessList(process **curr_process_list)
{
    if (!*curr_process_list)
        return;

    updateProcessList(curr_process_list);

    int index = 0;

    while (*curr_process_list)
    {
        printf("index: %d, PID: %d, status: %d, command: ", index, (*curr_process_list)->pid, (*curr_process_list)->status);
        printArr((*curr_process_list)->cmd);
        curr_process_list = &((*curr_process_list)->next);
        index++;
    }

    // delete all the terminated processes
    removeAllDeadProcesses(&process_list); // TODO: PROBLEM HERE

    return;
}

void performProcs()
{
    printProcessList(&process_list);
}

void printArr(cmdLine *pCmdLine)
{
    for (int i = 0; i < pCmdLine->argCount; ++i)
        printf("%s ", pCmdLine->arguments[i]);

    printf("\n");
}

void freeProcessList(process *process_list)
{
    freeCmdLines(process_list->cmd);
    free(process_list->next);
    free(process_list);
}

void updateProcessList(process **curr_process_list)
{
    if (!curr_process_list)
        return;

    int status;
    process *curr = *curr_process_list;

    while (curr)
    {
        int result = waitpid(curr->pid, &status, WNOHANG | WUNTRACED);
        if (result == 0)
        {
            // Process is still running
            curr->status = RUNNING;
        }
        else if (result == -1)
        {
            // Error occurred
            perror("waitpid");
        }
        else
        {
            // Process has changed state
            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                curr->status = TERMINATED;
            }
            else if (WIFSTOPPED(status))
            {
                curr->status = SUSPENDED;
            }
            else if (WIFCONTINUED(status))
            {
                curr->status = RUNNING;
            }
        }
        curr = curr->next;
    }
}

void updateProcessStatus(process *process_list, int pid, int status)
{
    if (!process_list)
        return;

    if ((*process_list).pid == pid)
    {
        (*process_list).status = status;
        return;
    }

    updateProcessStatus((*process_list).next, pid, status);
}

void cleanEnv()
{
    freeProcessList(process_list);
}

void removeAllDeadProcesses(process **curr_process_list)
{
    updateProcessList(curr_process_list);

    process *curr = *curr_process_list;
    process *prev = NULL;

    printf("PID\t\tCommand\t\tSTATUS\n");
    while (curr)
    {
        char *statusStr;
        switch (curr->status)
        {
        case RUNNING:
            statusStr = "Running";
            break;
        case SUSPENDED:
            statusStr = "Suspended";
            break;
        case TERMINATED:
            statusStr = "Terminated";
            break;
        default:
            statusStr = "Unknown";
            break;
        }

        printf("%d\t\t%s\t\t%s\n", curr->pid, curr->cmd->arguments[0], statusStr);

        if (curr->status == TERMINATED)
        {
            if (!prev)
            {
                *curr_process_list = curr->next;
                freeCmdLines(curr->cmd);
                free(curr);
                curr = *curr_process_list;
            }
            else
            {
                prev->next = curr->next;
                freeCmdLines(curr->cmd);
                free(curr);
                curr = prev->next;
            }
        }
        else
        {
            prev = curr;
            curr = curr->next;
        }
    }
}

void addToHistory(char *command)
{
    if (command[0] == '!' && command[1] == '!')
    {
        int index = newest - 1;
        if (index < 0)
            index += HISTLEN;

        char *lastCommand = history[index];

        if (lastCommand)
        {
            strncpy(history[newest], lastCommand, MAX_BUF - 1);
            history[newest][MAX_BUF - 1] = '\0';
            newest = (newest + 1) % HISTLEN;
            if (HistorySize < HISTLEN)
                ++HistorySize;
            else
                oldest = (oldest + 1) % HISTLEN;
        }
        else
            printf("There is no previous command.\n");
    }
    else if (command[0] == '!')
    {
        int index = atoi(command + 1);
        char *historyCommand = getCommandFromHistory(index);
        if (historyCommand)
        {
            strncpy(history[newest], historyCommand, MAX_BUF - 1);
            history[newest][MAX_BUF - 1] = '\0';
            newest = (newest + 1) % HISTLEN;
            if (HistorySize < HISTLEN)
                ++HistorySize;
            else
                oldest = (oldest + 1) % HISTLEN;
        }
        else
            printf("No such entry in this history index!\n");
    }
    else
    {
        strncpy(history[newest], command, MAX_BUF - 1);
        history[newest][MAX_BUF - 1] = '\0';
        newest = (newest + 1) % HISTLEN;
        if (HistorySize < HISTLEN)
            ++HistorySize;
        else
            oldest = (oldest + 1) % HISTLEN;
    }
}

void performHistory()
{
    int i = oldest;
    int count = 1;

    while (1)
    {
        printf("%d: %s\n", count++, history[i]);
        i = (i + 1) % HISTLEN;

        if (i == newest)
            return;
    }
}

void performLastCommand()
{
    int index = newest - 1;
    if (index < 0)
        index += HISTLEN;

    char *lastCommand = history[index];

    if (lastCommand)
    {
        cmdLine *parsedCommand = parseCmdLines(lastCommand);
        execute(parsedCommand);
    }
    else
        printf("no such command!");
}

void performTheNthCommand(cmdLine *currLine)
{
    char *numPointer = (&(*currLine).arguments[0][1]);
    int index = atoi(numPointer);

    if (index < 1 || index > HistorySize)
    {
        printf("Invalid history index\n");
        return;
    }

    int i = (oldest + index - 1) % HISTLEN;

    char *nthCommand = history[i];

    if (nthCommand)
    {
        cmdLine *parsedCommand = parseCmdLines(nthCommand);
        execute(parsedCommand);
    }
    else
        printf("no such command!");
}

char *getCommandFromHistory(int index)
{
    if (index < 1 || index > HistorySize)
    {
        printf("Invalid history index\n");
        return NULL;
    }
    int i = oldest + index - 1;
    if (i >= HISTLEN)
        i -= HISTLEN;

    return history[i];
}
