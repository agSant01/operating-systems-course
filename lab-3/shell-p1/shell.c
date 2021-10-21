#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define TOKEN_DELIMETERS " \t\r\n\a"
#define DEFAULT_SIZE 1024

char *history[DEFAULT_SIZE];
char *argv[DEFAULT_SIZE];
int hist_count = 0;
int hist_num = 0;
int position = 0;
short isHistNum = 0;

void read_line(char *line)
{
    if (isHistNum)
    {
        line = memcpy(line, history[hist_num - 1], DEFAULT_SIZE);
        printf("\n");
    }
    else
    {
        fgets(line, DEFAULT_SIZE, stdin);
    }
    isHistNum = 0;
    memcpy(history[hist_count], line, DEFAULT_SIZE);
    hist_count++;
}

void parse_line(char *line, char *argv[])
{
    char *token;
    position = 0;
    token = strtok(line, TOKEN_DELIMETERS);
    while (token != NULL)
    {
        argv[position] = token;
        position++;
        token = strtok(NULL, TOKEN_DELIMETERS);
    }
}

int nat_history(char *argv[])
{
    if (position == 2)
    {
        hist_num = atoi(argv[1]);
        for (int k = 1; k <= hist_count; k++)
        {
            if (hist_num == k)
            {
                isHistNum = 1;
            }
        }
    }
    if (isHistNum == 0)
    {
        for (int i = 0; i < hist_count; i++)
            printf(" %d %s\n", (i + 1), history[i]);
    }
    return 1;
}

void execute(char *argv[])
{

    // Check if command is valid as a native command
    char exit_cmd[10];
    strcpy(exit_cmd, "exit");
    char hist_cmd[10];
    strcpy(hist_cmd, "history");

    if (strcmp(argv[0], exit_cmd) == 0)
    {
        exit(0);
    }
    else if (strcmp(argv[0], hist_cmd) == 0)
    {
        nat_history(argv);
    }
    //Native commands
    else
    {
        //Execute any other command. Uses child process.
    }
}

int main(int argc, char *argv[])
{
    int valid = 0;
    char *line = (char *)malloc(DEFAULT_SIZE);
    for (int i = 0; i < DEFAULT_SIZE; i++)
        history[i] = (char *)malloc(DEFAULT_SIZE);
    long size;
    char *buf;
    char *ptr;

    printf("PID: %d\n", getpid());
    while (1)
    {
        printf("Shell->");
        read_line(line);
        parse_line(line, argv);
        execute(argv);
        for (int j = 0; j < DEFAULT_SIZE; j++)
        {
            printf("%d\n", j);
            argv[j] = NULL;
        }
    }
}