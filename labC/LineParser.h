#define MAX_ARGUMENTS 256

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

/* Parses a given string to arguments and other indicators */
/* Returns NULL when there's nothing to parse */
/* When successful, returns a pointer to cmdLine (in case of a pipe, this will be the head of a linked list) */
cmdLine *parseCmdLines(const char *strLine); /* Parse string line */

/* Releases all allocated memory for the chain (linked list) */
void freeCmdLines(cmdLine *pCmdLine); /* Free parsed line */

/* Replaces arguments[num] with newString */
/* Returns 0 if num is out-of-range, otherwise - returns 1 */
int replaceCmdArg(cmdLine *pCmdLine, int num, const char *newString);
