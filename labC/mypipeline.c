#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    int fd[2];
    pipe(fd);
    fprintf(stderr, "(parent_process>forking…)\n");
    pid_t pid1 = fork();
    if (pid1)
    {
        // parent
        fprintf(stderr, "(parent_process>created process with id: %d)\n", pid1);
        fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
        close(fd[1]);

        fprintf(stderr, "(parent_process>forking…)\n");
        pid_t pid2 = fork();
        if (pid2)
        {
            // parent
            fprintf(stderr, "(parent_process>created process with id: %d)\n", pid2);
            fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
            close(fd[0]);

            fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            fprintf(stderr, "(parent_process>exiting…)\n");
        }
        else
        {
            // child2
            close(STDIN_FILENO);
            fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            fprintf(stderr, "(child2>going to execute cmd: …)\n");
            execlp("tail", "tail", "-n", "2", NULL);
        }
    }
    else
    {
        // child1
        close(STDOUT_FILENO);
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        fprintf(stderr, "(child1>going to execute cmd: …)\n");
        execlp("ls", "ls", "-l", NULL);
    }
    return 0;
}
