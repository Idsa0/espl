#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char my_get(char c);
char cprt(char c);
char encrypt(char c);
char decrypt(char c);
char xoprt(char c);

char *map(char *array, int array_length, char (*f)(char))
{
    char *mapped_array = (char *)malloc(array_length * sizeof(char));
    /* TODO: Complete during task 2.a */
    for (int i = 0; i < array_length; i++)
        mapped_array[i] = f(array[i]);

    return mapped_array;
}

// int main(int argc, char **argv)
// {
//     /* TODO: Test your code */
//     char arr1[] = {'H', 'E', 'Y', '!'};
//     char *arr2 = map(arr1, 4, encrypt);
//     printf("%s\n", arr2);
//     free(arr2);

//     return 0;
// }

/* Ignores c, reads and returns a character from stdin using fgetc. */
char my_get(char c)
{
    return fgetc(stdin);
}

/* If c is a number between 0x20 and 0x7E, cprt prints the character of ASCII value c followed by a new line. Otherwise, cprt prints the dot ('.') character. After printing, cprt returns the value of c unchanged. */
char cprt(char c)
{
    if (c >= 0x20 && c <= 0x7E)
        printf("%i\n", c);
    else
        printf(".\n");

    return c;
}

/* Gets a char c. If c is between 0x20 and 0x4E add 0x20 to its value and return it. Otherwise return c unchanged */
char encrypt(char c)
{
    if (c >= 0x20 && c <= 0x4E)
        c += 0x20;

    return c;
}

/* Gets a char c and returns its decrypted form subtractng 0x20 from its value. But if c was not between 0x40 and 0x7E it is returned unchanged */
char decrypt(char c)
{
    if (c >= 0x40 && c <= 0x7E)
        c -= 0x20;

    return c;
}

/* xoprt prints the value of c in a hexadecimal representation, then in octal representation, followed by a new line, and returns c unchanged. */
char xoprt(char c)
{
    printf("%x %o\n", c, c);
    return c;
}
