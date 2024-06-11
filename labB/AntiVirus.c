#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHOICE_LEN 16
#define MAX_FILE_NAME 256
#define BUFFER_SIZE 10000
#define RET 0xC3 // return (near) @see https://c9x.me/x86/html/file_module_x86_id_280.html
#define VIRUS_SIGNATURE_LITTLE_ENDIAN "VIRL"
#define VIRUS_SIGNATURE_BIG_ENDIAN "VIRB"
#define ENDIAN_L 0
#define ENDIAN_B 1
#define DEFAULT_SIGNATURE_FILE_NAME "signatures-L"

int debug = 0;

void PrintHex(unsigned char buffer[], int length, FILE *out);

char sigFileName[MAX_FILE_NAME];
void SetSigFileName();
int endian = ENDIAN_L;

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

link *v_list = NULL;
FILE *sigFile = NULL;
FILE *suspectFile = NULL;
char suspectFileName[MAX_FILE_NAME];

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

void detect_virus(char *buffer, unsigned int size, link *virus_list);

typedef struct fun_desc
{
    char *name;
    void (*fun)(void);
} fun_desc;

void neutralize_virus(char *fileName, int signatureOffset);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <file> [-d]\n", argv[0]);
        return 1;
    }

    memcpy(suspectFileName, argv[1], strlen(argv[1]));
    suspectFile = fopen(argv[1], "rb");
    if (!suspectFile)
    {
        fprintf(stderr, "Error: cannot open file\n");
        return 1;
    }

    if (argc == 3 && strcmp(argv[2], "-d") == 0)
        debug = 1;

    strcpy(sigFileName, DEFAULT_SIGNATURE_FILE_NAME);
    sigFile = fopen(sigFileName, "rb");

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

    if (sigFile)
        fclose(sigFile);

    sigFile = fopen(sigFileName, "rb");
    if (!sigFile)
        fprintf(stderr, "Error: cannot open file\n");
}

virus *readVirus(FILE *file)
{
    if (feof(file))
        return NULL;

    virus *v = (virus *)malloc(sizeof(virus));
    if (!v)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        return NULL;
    }

    if (fread(v, 1, sizeof(short) + VIRUS_NAME_LENGTH, file) != sizeof(short) + VIRUS_NAME_LENGTH)
    {
        free(v);
        return NULL;
    }

    if (v->SigSize == 0)
    {
        free(v);
        return NULL;
    }

    if (endian)
        v->SigSize = ((v->SigSize) >> 8) | ((v->SigSize) << 8);

    if (v->SigSize > 1000 && debug)
        printf("Warning: signature size of %i is big\n", v->SigSize);

    v->sig = (unsigned char *)malloc(v->SigSize);
    if (!v->sig)
    {
        free(v);
        fprintf(stderr, "Error: memory allocation failed\n");
        return NULL;
    }

    if (fread(v->sig, 1, v->SigSize, file) != v->SigSize)
    {
        free(v->sig);
        free(v);
        return NULL;
    }

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
    fprintf(out, "\n");
    list_print(virus_list->nextVirus, out);
}

link *list_append(link *virus_list, virus *data)
{
    // this is done in backwards order since we are adding to the beginning of the list but the example has the list in the opposite order
    // not sure which way is required so I'm doing it this way

    link *newhead = (link *)malloc(sizeof(link));
    if (!newhead)
    {
        fprintf(stderr, "Error: memory allocation failed\n");
        return NULL;
    }

    newhead->vir = data;
    newhead->nextVirus = virus_list;
    return newhead;
}

void list_free(link *virus_list)
{
    if (!virus_list)
        return;

    virus_free(virus_list->vir);
    list_free(virus_list->nextVirus);
    free(virus_list);
    virus_list = NULL;
}

void LoadSignatures()
{
    if (!sigFile)
    {
        fprintf(stderr, "Error: no file loaded\n");
        return;
    }

    if (v_list)
        list_free(v_list);

    v_list = load_signatures(sigFile);
}

link *load_signatures(FILE *file)
{
    // check if the file is starts with the magic numbers for virus signatures
    char sig[5] = {0};
    fread(sig, 1, 4, file);
    if (strcmp(sig, VIRUS_SIGNATURE_LITTLE_ENDIAN) == 0)
        endian = 0;
    else if (strcmp(sig, VIRUS_SIGNATURE_BIG_ENDIAN) == 0)
        endian = 1;
    else
    {
        fprintf(stderr, "Error: no virus signature\n");
        return NULL;
    }

    virus *v;
    link *head = NULL;
    while ((v = readVirus(file)))
        head = list_append(head, v);

    fseek(file, 0, SEEK_SET); // reset the file pointer
    return head;
}

void PrintSignatures()
{
    list_print(v_list, stdout);
}

void DetectViruses()
{
    if (!suspectFile)
    {
        fprintf(stderr, "Error: no file loaded\n");
        return;
    }
    if (!v_list)
    {
        fprintf(stderr, "Error: no signatures loaded\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    int read;
    if (!(read = fread(buffer, 1, sizeof(buffer), suspectFile)))
    {
        fprintf(stderr, "Error: cannot read file\n");
        return;
    }

    detect_virus(buffer, read, v_list);
    fseek(suspectFile, 0, SEEK_SET); // reset the file pointer
}

void detect_virus(char *buffer, unsigned int size, link *virus_list)
{
    if (!virus_list)
        return;

    virus *v = virus_list->vir;

    for (int i = 0; i < size; ++i)
        if (memcmp(buffer + i, v->sig, v->SigSize) == 0)
            printf("Virus detected: %s, at offset %d\n", v->virusName, i);

    detect_virus(buffer, size, virus_list->nextVirus);
}

void FixFile()
{
    if (!suspectFile)
        suspectFile = fopen(suspectFileName, "rb");

    if (!suspectFile)
    {
        fprintf(stderr, "Error: no file loaded\n");
        return;
    }
    if (!v_list)
    {
        fprintf(stderr, "Error: no signatures loaded\n");
        return;
    }

    char buffer[BUFFER_SIZE] = {0};
    int read;
    if (!(read = fread(buffer, 1, sizeof(buffer), suspectFile)))
    {
        fprintf(stderr, "Error: cannot read file\n");
        return;
    }

    fclose(suspectFile);
    suspectFile = NULL;

    link *curr = v_list;
    while (curr)
    {
        virus *v = curr->vir;
        for (int i = 0; i < read; ++i)
            if (memcmp(buffer + i, v->sig, v->SigSize) == 0)
            {
                printf("Neutralizing: %s, at offset %d\n", v->virusName, i);
                neutralize_virus(suspectFileName, i);
            }

        curr = curr->nextVirus;
    }
    suspectFile = fopen(suspectFileName, "rb");
}

void neutralize_virus(char *fileName, int signatureOffset)
{
    FILE *file = fopen(fileName, "rb+");
    if (!file)
    {
        fprintf(stderr, "Error: cannot open file\n");
        return;
    }

    if (fseek(file, signatureOffset, SEEK_SET))
    {
        fprintf(stderr, "Error: cannot seek to offset\n");
        fclose(file);
        return;
    }

    if (fputc(RET, file) == EOF)
    {
        fprintf(stderr, "Error: cannot write to file\n");
        fclose(file);
        return;
    }

    fclose(file);
}

void quit()
{
    list_free(v_list);
    if (suspectFile)
        fclose(suspectFile);
    if (sigFile)
        fclose(sigFile);

    exit(EXIT_SUCCESS);
}
