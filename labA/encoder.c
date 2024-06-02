#include <stdio.h>
#include <string.h>

char encode(char c, int key, int sign);

int main(int argc, char **argv)
{
    int i, debug = 0, sign = 1, c;
    char *eKey = NULL;
    FILE *infile = stdin;
    FILE *outfile = stdout;

    for (i = 1; i < argc; i++)
    {
        // kwargs for PART 1
        if (strcmp(argv[i], "+D") == 0)
            debug = 1;
        else if (strcmp(argv[i], "-D") == 0)
            debug = 0;

        // kwargs for PART 2
        else if (argv[i][0] == '+' && argv[i][1] == 'e' && argv[i][2] != 0)
        {
            eKey = argv[i] + 2;
            sign = 1;
        }
        else if (argv[i][0] == '-' && argv[i][1] == 'e' && argv[i][2] != 0)
        {
            eKey = argv[i] + 2;
            sign = -1;
        }

        // kwargs for PART 3
        else if (argv[i][0] == '-' && argv[i][1] == 'I' && argv[i][2] != 0)
        {
            infile = fopen(argv[i] + 2, "r");
            if (infile == NULL) // TODO does this work?
            {
                fprintf(stderr, "CANNOT OPEN FILE\n");
                return -1;
            }
        }
        else if (argv[i][0] == '-' && argv[i][1] == 'O' && argv[i][2] != 0)
        {
            outfile = fopen(argv[i] + 2, "w");
            if (outfile == NULL) // TODO does this work?
            {
                fprintf(stderr, "CANNOT OPEN FILE\n");
                return -1;
            }
        }

        if (debug)
            fprintf(stderr, "%s\n", argv[i]);
    }

    // assuming we don't want an encoding key with letters:
    if (eKey != NULL)
    {
        i = 0;
        while (eKey[i])
        {
            if (eKey[i] < '0' || eKey[i] > '9')
            {
                eKey = NULL;
                break;
            }
            ++i;
        }
    }

    i = 0;
    while (1)
    {
        while ((c = fgetc(infile)) != '\n')
            if (feof(infile))
            {
                if (outfile == stdout && infile != stdin)
                    printf("\n");
                fclose(infile);
                fclose(outfile);
                return 0;
            }
            else if (eKey == NULL)
                fputc((char)c, outfile);
            else
            {
                fputc(encode((char)c, eKey[i] - '0', sign), outfile);
                if (!eKey[++i])
                    i = 0;
            }

        // breaking from the loop still increments the key
        if (eKey != NULL && !eKey[++i])
            i = 0;

        fputc('\n', outfile);
    }

    return 0;
}

/* Uses 
*/
char encode(char c, int key, int sign)
{
    if (c >= '0' && c <= '9')
        return ((c - '0' + sign * key + 10) % 10) + '0';
    
    /*
        do not apply to uppercase
        if (c >= 'A' && c <= 'Z')
            return ((c - 'A' + sign * key + 26) % 26) + 'A';
    */

    if (c >= 'a' && c <= 'z')
        return ((c - 'a' + sign * key + 26) % 26) + 'a';

    return c;
}
