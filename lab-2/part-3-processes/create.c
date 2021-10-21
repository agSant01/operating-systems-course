/* Example of use of fork system call */
#include <stdio.h>
#include <stdlib.h>
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
        execlp("/bin/ps", "ps", NULL);
        printf("Still around...\n");
    }
    else
    {
        exit(0);
    }
}