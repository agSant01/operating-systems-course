#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#define TOKEN_DELIMETERS " \t\r\n\a"
#define DEFAULT_SIZE 1024

char *history[DEFAULT_SIZE];
char *argv[DEFAULT_SIZE];
int hist_count = 0;
int hist_num = 0;
int position = 0;
short isHistNum = 0;

/**
 * @brief Helper function to compare two strings ignoring whitespace
 * 
 * @param str1 
 * @param str2 
 * @return int 
 */
int areEqual(char *str1, char *str2)
{
  int i1 = 0, i2 = 0;
  u_int lenStr1 = strlen(str1);
  u_int lenStr2 = strlen(str2);

  int result = 1;
  while (i1 < lenStr1 && i2 < lenStr2)
  {
    if (str1[i1] == ' ')
    {
      i1++;
      continue;
    }

    if (str2[i2] == ' ')
    {
      i2++;
      continue;
    }

    result = result && str1[i1++] == str2[i2++];

    if (result == 0)
      return 0;
  }

  return result && (lenStr1 == lenStr2);
}

void read_line(char *line)
{
  if (isHistNum)
  {
    line = memcpy(line, history[hist_num], DEFAULT_SIZE);
    printf("%s\n", line);
  }
  else
  {
    gets(line);
  }

  isHistNum = 0;
  if (strlen(line) > 0)
  {
    memcpy(history[hist_count], line, DEFAULT_SIZE);
    hist_count++;
  }
}

void parse_line(char *line, char *arguments[])
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
    for (int k = 0; k < hist_count; k++)
    {
      if (hist_num == k)
      {
        char *cmp = (char *)malloc(100);
        strcpy(cmp, "history ");
        strcat(cmp, argv[1]);
        if (areEqual(cmp, history[k]) == 0)
        {
          isHistNum = 1;
        }
        free(cmp);
      }
    }
  }
  if (isHistNum == 0)
  {
    for (int i = 0; i < hist_count; i++)
      printf(" %d %s\n", i, history[i]);
  }
  return 1;
}

int nat_help(char *argv[])
{
  // help
  printf("Usage: Interactive shell that executes natively 4 commands and supports all HOST OS's by ");
  printf("using the system call <execvp(...)> to execute the HOST OS's programs.\n\n");
  printf("Native Commands\n");
  printf("Usage: <command> [options...]\n");
  printf("    help                   show this message\n");
  printf("    history [<id>]         show a list of the commands executed in the shell with their <id>; If <id> is provided that command will be executed.\n");
  printf("    cd                     change current working directory\n");
  printf("    exit                   closes the interactive shell\n");
  printf("\n");
  printf("For ICOM5007 - Operating Systems Course Lab 3: Shells & PThreads\n");
  printf("Gabriel S. Santiago | Email: <gabriel.santiago16@upr.edu>\n");

  return 0;
}

void execute(char *argv[])
{
  // no args
  if (argv[0] == NULL)
    return;

  // Check if command is valid as a native command
  char exit_cmd[10];
  strcpy(exit_cmd, "exit");

  char hist_cmd[10];
  strcpy(hist_cmd, "history");

  char cd_cmd[10];
  strcpy(cd_cmd, "cd");

  char help_cmd[10];
  strcpy(help_cmd, "help");

  // Native commands
  if (strcmp(argv[0], exit_cmd) == 0)
  {
    exit(0);
  }
  else if (strcmp(argv[0], hist_cmd) == 0)
  {
    nat_history(argv);
  }
  else if (strcmp(argv[0], help_cmd) == 0)
  {
    nat_help(argv);
  }
  else if (strcmp(argv[0], cd_cmd) == 0)
  {
    if (argv[1] == NULL || strcmp(argv[1], "~") == 0)
    {
      argv[1] = getenv("HOME");
    }

    chdir(argv[1]);
  }
  // end of native commands
  else
  {
    // Execute any other command. Uses child process.
    pid_t pid = fork();

    if (pid == 0)
    {
      // child
      execvp(argv[0], argv);
      exit(0);
    }

    waitpid(pid, NULL, 0);
  }
}

int main()
{
  char *line = (char *)malloc(DEFAULT_SIZE);

  for (int i = 0; i < DEFAULT_SIZE; i++)
  {
    history[i] = (char *)malloc(DEFAULT_SIZE);
    argv[i] = (char *)malloc(DEFAULT_SIZE);
  }

  fprintf(stderr, "Shell PID: %d\n", getpid());

  // Generate prompt
  char *userpc = getenv("USER");
  char hostname[60];

  gethostname(hostname, sizeof(hostname));

  strcat(userpc, "@");
  strcat(userpc, hostname);

  while (1)
  {
    // clear argv array
    bzero(argv, DEFAULT_SIZE);

    printf("\e[1;35m%s\e[1;0m:\e[1;36m%s$\e[1;0m ", userpc, getcwd(NULL, 0));
    read_line(line);
    parse_line(line, argv);
    execute(argv);
  }

  return 0; // success
}