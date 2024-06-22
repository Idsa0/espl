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
#include <ctype.h>

#define MAX_ARGUMENTS 256

#ifndef NULL
#define NULL 0
#endif

#define FREE(X) \
    if (X)      \
    free((void *)X)

typedef struct cmdLine
{
    char *const arguments[MAX_ARGUMENTS]; /* command line arguments (arg 0 is the command)*/
    int argCount;                         /* number of arguments */
    char const *inputRedirect;            /* input redirection path. NULL if no input redirection */
    char const *outputRedirect;           /* output redirection path. NULL if no output redirection */
    char blocking;                        /* boolean indicating blocking/non-blocking */
    int idx;                              /* index of current command in the chain of cmdLines (0 for the first) */
    struct cmdLine *next;                 /* next cmdLine in chain */
} cmdLine;

static char *cloneFirstWord(char *str)
{
    char *start = NULL;
    char *end = NULL;
    char *word;

    while (!end)
    {
        switch (*str)
        {
        case '>':
        case '<':
        case 0:
            end = str - 1;
            break;
        case ' ':
            if (start)
                end = str - 1;
            break;
        default:
            if (!start)
                start = str;
            break;
        }
        str++;
    }

    if (start == NULL)
        return NULL;

    word = (char *)malloc(end - start + 2);
    strncpy(word, start, ((int)(end - start) + 1));
    word[(int)((end - start) + 1)] = 0;

    return word;
}

static void extractRedirections(char *strLine, cmdLine *pCmdLine)
{
    char *s = strLine;

    while ((s = strpbrk(s, "<>")))
    {
        if (*s == '<')
        {
            FREE(pCmdLine->inputRedirect);
            pCmdLine->inputRedirect = cloneFirstWord(s + 1);
        }
        else
        {
            FREE(pCmdLine->outputRedirect);
            pCmdLine->outputRedirect = cloneFirstWord(s + 1);
        }

        *s++ = 0;
    }
}

static char *strClone(const char *source)
{
    char *clone = (char *)malloc(strlen(source) + 1);
    strcpy(clone, source);
    return clone;
}

static int isEmpty(const char *str)
{
    if (!str)
        return 1;

    while (*str)
        if (!isspace(*(str++)))
            return 0;

    return 1;
}

static cmdLine *parseSingleCmdLine(const char *strLine)
{
    char *delimiter = " ";
    char *line, *result;

    if (isEmpty(strLine))
        return NULL;

    cmdLine *pCmdLine = (cmdLine *)malloc(sizeof(cmdLine));
    memset(pCmdLine, 0, sizeof(cmdLine));

    line = strClone(strLine);

    extractRedirections(line, pCmdLine);

    result = strtok(line, delimiter);
    while (result && pCmdLine->argCount < MAX_ARGUMENTS - 1)
    {
        ((char **)pCmdLine->arguments)[pCmdLine->argCount++] = strClone(result);
        result = strtok(NULL, delimiter);
    }

    FREE(line);
    return pCmdLine;
}

static cmdLine *_parseCmdLines(char *line)
{
    char *nextStrCmd;
    cmdLine *pCmdLine;
    char pipeDelimiter = '|';

    if (isEmpty(line))
        return NULL;

    nextStrCmd = strchr(line, pipeDelimiter);
    if (nextStrCmd)
        *nextStrCmd = 0;

    pCmdLine = parseSingleCmdLine(line);
    if (!pCmdLine)
        return NULL;

    if (nextStrCmd)
        pCmdLine->next = _parseCmdLines(nextStrCmd + 1);

    return pCmdLine;
}

/* Parses a given string to arguments and other indicators */
/* Returns NULL when there's nothing to parse */
/* When successful, returns a pointer to cmdLine (in case of a pipe, this will be the head of a linked list) */
cmdLine *parseCmdLines(const char *strLine) /* Parse string line */
{
    char *line, *ampersand;
    cmdLine *head, *last;
    int idx = 0;

    if (isEmpty(strLine))
        return NULL;

    line = strClone(strLine);
    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = 0;

    ampersand = strchr(line, '&');
    if (ampersand)
        *(ampersand) = 0;

    if ((last = head = _parseCmdLines(line)))
    {
        while (last->next)
            last = last->next;
        last->blocking = ampersand ? 0 : 1;
    }

    for (last = head; last; last = last->next)
        last->idx = idx++;

    FREE(line);
    return head;
}

/* Releases all allocated memory for the chain (linked list) */
void freeCmdLines(cmdLine *pCmdLine) /* Free parsed line */
{
    int i;
    if (!pCmdLine)
        return;

    FREE(pCmdLine->inputRedirect);
    FREE(pCmdLine->outputRedirect);
    for (i = 0; i < pCmdLine->argCount; ++i)
        FREE(pCmdLine->arguments[i]);

    if (pCmdLine->next)
        freeCmdLines(pCmdLine->next);

    FREE(pCmdLine);
}

/* Replaces arguments[num] with newString */
/* Returns 0 if num is out-of-range, otherwise - returns 1 */
int replaceCmdArg(cmdLine *pCmdLine, int num, const char *newString)
{
    if (num >= pCmdLine->argCount)
        return 0;

    FREE(pCmdLine->arguments[num]);
    ((char **)pCmdLine->arguments)[num] = strClone(newString);
    return 1;
}

int debug = 0;

#define MAX_LINE 2048

typedef struct history
{
    char *command;
    struct history *next;
} history;

history *hist = NULL;

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

history *add_to_history(char *command)
{
    history *new = (history *)malloc(sizeof(history));

    if (!new)
    {
        perror("Error");
        return hist;
    }

    new->command = strdup(command);
    new->next = hist;

    return new;
}

void free_history()
{
    history *tmp;
    while (hist)
    {
        tmp = hist;
        hist = hist->next;
        free(tmp->command);
        free(tmp);
    }
}

int chkspccmds(cmdLine *line)
{
    if (strcmp(line->arguments[0], "quit") == 0)
    {
        freeCmdLines(line);
        free_history();
        exit(EXIT_SUCCESS);
    }
    
    return 0;
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

        if (!line)
        {
            if (feof(stdin))
            {
                free_history();
                exit(EXIT_SUCCESS);
            }

            fprintf(stderr, "Error: invalid command\n");
            // TODO check for add_to_history
        }
        else if (!line->argCount)
        {
            fprintf(stderr, "Error: invalid command\n");
            freeCmdLines(line);
            // TODO check for add_to_history
        }
        else if (chkspccmds(line))
        {
            hist = add_to_history(buf);
            freeCmdLines(line);
            line = NULL;
        }
        else
        {
            execute(line);
            hist = add_to_history(buf);
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
