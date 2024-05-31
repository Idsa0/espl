#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIRUS_NAME_LENGTH 16

typedef struct virus
{
    unsigned short SigSize;
    char virusName[VIRUS_NAME_LENGTH];
    unsigned char *sig;
} virus;

void freeVirus(virus *v)
{
    free(v->sig);
    free(v);
}

void PrintHex(unsigned char buffer[], int length)
{
    int i;
    for (i = 0; i < length - 1; ++i)
        printf("%02X ", buffer[i]);

    if (length > 0)
        printf("%02X\n", buffer[length - 1]);
}

char sigFileName[256] = {0};

void SetSigFileName()
{
    printf("Enter signature file name: ");
    fgets(sigFileName, sizeof(sigFileName), stdin);
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
    if (!virus)
        return;

    printf("Virus name: %s\n", virus->virusName);
    printf("Virus size: %d\n", virus->SigSize);
    printf("signature:\n");
    PrintHex(virus->sig, virus->SigSize);
}

int main(int argc, char **argv)
{
    if (argc < 2) // instructions say to use argv[1] as the file name but we can also ask the user for the file name
    {
        strcpy(sigFileName, "signatures-L");
        SetSigFileName();
    }
    else
        strcpy(sigFileName, argv[1]);

    FILE *file = fopen(sigFileName, "rb");
    if (!file)
    {
        printf("Error: cannot open file %s\n", argv[1]);
        return 1;
    }

    // check if the file is starts with the magic numbers VIRL or VIRB
    char sig[5] = {0};
    fread(sig, 1, 4, file);
    if (strcmp(sig, "VIRL") != 0 && strcmp(sig, "VIRB") != 0)
    {
        printf("Error: no virus detected\n");
        return 1;
    }

    virus *v;
    while (!feof(file))
    {
        v = readVirus(file);
        printVirus(v);
        freeVirus(v);
    }

    fclose(file);
    return 0;
}
