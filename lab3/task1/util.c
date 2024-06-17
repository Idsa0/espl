/*
string-related utility functions for lab4
http://www.cs.bgu.ac.il/~caspl152/Lab4/Tasks
*/

/*
Returns the length of str.
The length of a C string is determined by the terminating null-character:
A C string is as long as the amount of characters between the beginning
of the string and the terminating null character.
example: strlen("abc") return 3
*/
unsigned int strlen(const char *str)
{
    int i = 0;
    while (str[i])
    {
        ++i;
    }
    return i;
}

/* in a 32-bit system, the maximal value of an int is 2^31 - 1 or 2,147,483,648.
 * This means that 10 digits are enough + 1 char for the terminating '\0' + 1 char for '-'.
 * in case of xtoa, it is 8 hex digits + 2 for the "0x" prefix.
 */
#define BUFFER_SIZE 12

/* we define the buffer as a global variable so we can return
 * a pointer into it: local variables are saved on the stack
 * and should never be refrenced outside the function!
 */
char buffer[BUFFER_SIZE];

/* returns the decimal representation of a _signed_ integer */
char *itoa(int num)
{
    char *p = buffer + BUFFER_SIZE - 1;
    int neg = num < 0;

    if (neg)
    {
        num = -num;
    }

    *p = '\0';
    do
    {
        *(--p) = '0' + num % 10;
    } while (num /= 10);

    if (neg)
    {
        *(--p) = '-';
    }

    return p;
}

/*
Compares the C string str1 to the C string str2.
This function starts comparing the first character of each string. If they are
equal to each other, it continues with the following pairs until the characters
differ or until a terminating null-character is reached.
*/
int strcmp(const char *str1, const char *str2)
{
    while (*str1 == *str2 && *str1)
    {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

/*
The strncmp() function is similar to strcmp,
except it only compares the first (at most) n characters of s1 and s2.
*/
int strncmp(const char *str1, const char *str2, unsigned int n)
{
    while (n--)
    {
        if (*str1++ != *str2++)
        {
            return *(unsigned char *)(str1 - 1) - *(unsigned char *)(str2 - 1);
        }
    }
    return 0;
}

/*
Return the int value of the positive integer written in the string str or -1 on errors.
Doesn't support negative integers or non-decimal formats.
*/
int positive_atoi(char *str)
{
    int ret = 0, i;
    for (i = 0; str[i] != 0; i++)
    {
        if (str[i] < '0' || str[i] > '9')
            return -1;
        ret *= 10;
        ret += str[i] - '0';
    }
    return ret;
}
