#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHOICE_LEN 16
#define MAX_SIG_FILE_NAME 256

void PrintHex(unsigned char buffer[], int length, FILE *out);

char sigFileName[MAX_SIG_FILE_NAME] = {0};
void SetSigFileName();

#define VIRUS_NAME_LENGTH 16

typedef struct virus
{
    unsigned short SigSize;
    char virusName[VIRUS_NAME_LENGTH];
    unsigned char *sig;
} virus;

virus *readVirus(FILE *file);
void printVirus(virus *virus);
void print_virus_to_file(virus *virus, FILE *out);
void virus_free(virus *v);

typedef struct link link;

struct link
{
    link *nextVirus;
    virus *vir;
};

link *virus_list = NULL;
FILE *file = NULL;

/* Print the data of every link in list to the given stream. Each item followed by a newline character. */
void list_print(link *virus_list, FILE *);
/* Add a new link with the given data at the beginning of the list and return a pointer to the list (i.e., the first link in the list).
If the list is null - create a new entry and return a pointer to the entry.
 */
link *list_append(link *virus_list, virus *data);
/* Free the memory allocated by the list. */
void list_free(link *virus_list);

void LoadSignatures();
link *load_signatures(FILE *file);
void PrintSignatures();
void DetectViruses();
void FixFile();
void quit();

typedef struct fun_desc
{
    char *name;
    void (*fun)(void);
} fun_desc;

int main(int argc, char **argv)
{
    int debug = 0;
    if (argc > 1 && strcmp(argv[1], "-D") == 0)
        debug = 1;

    strcpy(sigFileName, "signatures-L"); // default file name
    file = fopen(sigFileName, "rb");

    fun_desc functions[] = {
        {"Set signatures file name", SetSigFileName},
        {"Load signatures", LoadSignatures},
        {"Print signatures", PrintSignatures},
        {"Detect viruses", DetectViruses},
        {"Fix file", FixFile},
        {"Quit", quit}};

    int bound = sizeof(functions) / sizeof(functions[0]);
    int choice;
    char input[MAX_CHOICE_LEN];
    char *endptr;
    while (!feof(stdin))
    {
        for (int i = 0; i < bound; i++)
            printf("%i: %s\n", i, functions[i].name);

        if (fgets(input, MAX_CHOICE_LEN, stdin) != NULL)
        {
            choice = strtol(input, &endptr, 10);
            if (endptr == input || *endptr != '\n')
            {
                // invalid input - for now we do nothing and continue
                printf("Invalid input\n");
            }
            else if (choice < 0 || choice >= bound)
            {
                printf("Not within bounds\n");
            }
            else
            {
                functions[choice].fun();
            }
        }
    }
}

void PrintHex(unsigned char buffer[], int length, FILE *out)
{
    for (int i = 0; i < length - 1; ++i)
        fprintf(out, "%02X ", buffer[i]);

    if (length > 0)
        fprintf(out, "%02X\n", buffer[length - 1]);
}

void SetSigFileName()
{
    printf("Enter signature file name: ");
    fgets(sigFileName, sizeof(sigFileName), stdin);
    sigFileName[strcspn(sigFileName, "\n")] = 0; // strip the newline character
    file = fopen(sigFileName, "rb");
    if (!file)
        printf("Error: cannot open file\n");
}

virus *readVirus(FILE *file)
{
    if (feof(file))
        return NULL;

    virus *v = (virus *)malloc(sizeof(virus));
    fread(v, 1, sizeof(short) + VIRUS_NAME_LENGTH, file);
    v->sig = (unsigned char *)malloc(v->SigSize);
    fread(v->sig, 1, v->SigSize, file);
    return v;
}

void printVirus(virus *virus)
{
    print_virus_to_file(virus, stdout);
}

void print_virus_to_file(virus *virus, FILE *out)
{
    if (!virus)
        return;

    fprintf(out, "Virus name: %s\n", virus->virusName);
    fprintf(out, "Virus size: %d\n", virus->SigSize);
    fprintf(out, "signature:\n");
    PrintHex(virus->sig, virus->SigSize, out);
}

void virus_free(virus *v)
{
    free(v->sig);
    free(v);
}

void list_print(link *virus_list, FILE *out)
{
    if (!virus_list)
        return;

    print_virus_to_file(virus_list->vir, out);
    list_print(virus_list->nextVirus, out);
}

link *list_append(link *virus_list, virus *data)
{
    link *next = (link *)malloc(sizeof(link));
    next->vir = data;
    next->nextVirus = virus_list;
    return next;
}

void list_free(link *virus_list)
{
    if (!virus_list)
        return;

    virus_free(virus_list->vir);
    list_free(virus_list->nextVirus);
    free(virus_list);
}

void LoadSignatures()
{
    if (file)
        virus_list = load_signatures(file);
    else
        printf("Error: no file loaded\n");
}

link *load_signatures(FILE *file)
{
    // check if the file is starts with the magic numbers VIRL or VIRB
    char sig[5] = {0};
    fread(sig, 1, 4, file);
    if (strcmp(sig, "VIRL") != 0 && strcmp(sig, "VIRB") != 0)
    {
        printf("Error: no virus detected\n");
        return NULL;
    }

    virus *v;
    link *head = NULL;
    while ((v = readVirus(file)))
        head = list_append(head, v);

    return head;
}

void PrintSignatures()
{
    list_print(virus_list, stdout);
}

void DetectViruses()
{
    printf("NOT YET IMPLEMENTED\n");
}

void FixFile()
{
    printf("NOT YET IMPLEMENTED\n");
}

void quit()
{
    list_free(virus_list);
    if (file)
        fclose(file);

    exit(0);
}
