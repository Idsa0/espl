#include <stdio.h>

#define isdigit(c) ((c) >= '0' && (c) <= '9')

int count_digits(const char *str)
{
    int count = 0;
    while (*str)
    {
        if (isdigit(*str))
            ++count;
        ++str;
    }
    return count;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        return 1;
    }

    printf("%d\n", count_digits(argv[1]));
    return 0;
}
