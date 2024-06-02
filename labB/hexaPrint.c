#include <stdio.h>
#include <stdlib.h>

void bubbleSort(int numbers[], int array_size)
{
    int i, j, temp;
    for (i = (array_size - 1); i > 0; i--)
    {
        for (j = 1; j <= i; j++)
        {
            if (numbers[j - 1] > numbers[j])
            {
                temp = numbers[j - 1];
                numbers[j - 1] = numbers[j];
                numbers[j] = temp;
            }
        }
    }
}

void PrintHex(int buffer[], int length)
{
    int i;
    for (i = 0; i < length - 1; ++i)
        printf("%02X ", buffer[i]);

    if (length > 0)
        printf("%02X\n", buffer[length - 1]);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file)
    {
        printf("Error: cannot open file %s\n", argv[1]);
        return 1;
    }

    char c;
    while (fread(&c, sizeof(char), 1, file) == 1)
    {
        printf("%x ", (unsigned char)c);
    }
    printf("\n");

    fclose(file);
    return 0;
}
