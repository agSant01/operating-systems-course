/* Example of use of fork system call */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void main()
{
    int pid;
    pid = fork();

    if (pid < 0)
    {
        fprintf(stderr, "Fork failed!\n");
        exit(-1);
    }
    else if (pid == 0)
    {
        printf("I am the CHILD | My PID: %d | Parent PID: %d\n", getpid(), getppid());
        execlp("/bin/ps", "ps", NULL);
        printf("Still around...\n");
    }
    else
    {
        waitpid(pid, NULL, WUNTRACED);
        printf("I am the PARENT | My Child PID: %d\n", pid);
        exit(0);
    }
}
