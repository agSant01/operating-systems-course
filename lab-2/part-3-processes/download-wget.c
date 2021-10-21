#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int main()
{
    char command[300];

    //copy the command to download the file
    const char *url = "https://www.c-programming-simple-steps.com/support-files/return-statement.zip";

    strcpy(command, "wget --quiet ");
    strcat(command, url);

    //execute the command
    int wget_ret = system(command);

    if (wget_ret == -1)
    {
        fprintf(stderr, "> Error ocurred when executing wget... Fork failed...\n");
        exit(1);
    }
    else if (WIFEXITED(wget_ret))
    {
        fprintf(stderr, "> Wget terminated normally...\n");
        fprintf(stderr, "> Wget exit status: %d\n", WEXITSTATUS(wget_ret));
    }

    //copy the commmand to list the files
    strcpy(command, "ls | grep return");

    //execute the command
    int ls_ret = system(command);

    if (ls_ret == -1)
    {
        fprintf(stderr, "> Error ocurred when executing \"ls | grep\"... Fork failed...\n");
        exit(1);
    }
    else if (WIFEXITED(ls_ret))
    {
        fprintf(stderr, "> \"ls | grep\" terminated normally...\n");
        fprintf(stderr, "> \"ls | grep\" exit status: %d\n", WEXITSTATUS(ls_ret));
    }

    return 0;
}