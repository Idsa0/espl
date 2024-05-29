#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct fun_desc
{
    char *name;
    char (*fun)(char);
} fdesc;

char *map(char *array, int array_length, char (*f)(char));
char my_get(char c);
char cprt(char c);
char encrypt(char c);
char decrypt(char c);
char xoprt(char c);

int main()
{
    fdesc funcs[] = {{"my_get", my_get}, {"cprt", cprt}, {"encrypt", encrypt}, {"decrypt", decrypt}, {"xoprt", xoprt}, {NULL, NULL}};
    char carray[5] = {0};

    printf("Select operation from the following menu:\n");
    int bound = sizeof(funcs) / sizeof(funcs[0]);
    for (int i = 0; i < bound; i++)
        printf("%i: %s\n", i, funcs[i].name);

    char input[1024];
    int choice;
    char *endptr;
    while (!feof(stdin))
    {
        if (fgets(input, 1024, stdin) != NULL)
        {
            choice = strtol(input, &endptr, 10);
            if (endptr == input || *endptr != '\n')
            {
                // invalid input - if the user is wrong we quit
                return 0;
            }
            if (choice < 0 || choice >= bound) // note - the user can choose (null)
            {
                printf("Not within bounds\n");
                return 0;
            }
            else
            {
                printf("Within bounds\n");
                char *temp = map(carray, 5, funcs[choice].fun);
                memcpy(carray, temp, 5);
                free(temp);
            }
        }
    }

    // free(carray);
    return 0;
}
