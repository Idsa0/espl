#include "util.h"

#define SYS_WRITE 4
#define STDOUT 1
#define SYS_OPEN 5
#define SYS_CLOSE 6
#define O_RDWR 2
#define SYS_SEEK 19
#define SEEK_SET 0
#define SHIRA_OFFSET 0x291

char *filename = 0;

extern int system_call();

void infection();
void infector(char *filename);

int main(int argc, char *argv[], char *envp[])
{
    if (!argc)
        return 0x55;

    int i;
    for (i = 1; i < argc; ++i)
    {
        if (!strncmp(argv[i], "-a", 2))
        {
            filename = argv[i] + 2;
            system_call(SYS_WRITE, STDOUT, filename, strlen(filename));
            system_call(SYS_WRITE, STDOUT, "\n", 1);
            break;
        }
    }

    infection();

    if (filename)
        infector(filename);

    return 0;
}
