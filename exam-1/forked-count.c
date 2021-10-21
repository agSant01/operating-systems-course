#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int global = 0;
int main()
{
    while (!fork())
    {
        sleep(1);
        if (global < 5 && fork())
        {
            global += 1;
            wait(NULL);
        }
        else
            exit(0);
    }
    printf("\%d\n", global);
    return 0;
}
