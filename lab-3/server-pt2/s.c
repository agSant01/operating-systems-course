#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/un.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

// Prototypes for internal functions and utilities
void error(const char *fmt, ...);
int runClient(char *clientName, int numMessages, char **messages);
int runServer();
void serverReady(int signal);

pthread_t tidServer[2];

bool serverIsReady = false;

// Prototypes for functions to be implemented by students
void server();
void client(char *clientName, int numMessages, char *messages[]);

void verror(const char *fmt, va_list argp)
{
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n");
}

void error(const char *fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    verror(fmt, argp);
    va_end(argp);
    exit(1);
}

int runServer(int port)
{
    int forkPID = fork();
    if (forkPID < 0)
        error("ERROR forking server");
    else if (forkPID == 0)
    {
        server();
        exit(0);
    }
    else
    {
        printf("Main: Server(%d) launched...\n", forkPID);
    }
    return forkPID;
}

int runClient(char *clientName, int numMessages, char **messages)
{
    fflush(stdout);
    printf("Launching client %s...\n", clientName);
    int forkPID = fork();
    if (forkPID < 0)

        error("ERROR forking client %s", clientName);
    else if (forkPID == 0)
    {
        client(clientName, numMessages, messages);
        exit(0);
    }
    return forkPID;
}

void serverReady(int signal)
{
    serverIsReady = true;
}

#define NUM_CLIENTS 2
#define MAX_MESSAGES 5
#define MAX_CLIENT_NAME_LENGTH 10

struct client
{
    char name[MAX_CLIENT_NAME_LENGTH];
    int num_messages;
    char *messages[MAX_MESSAGES];
    int PID;
};

// Modify these to implement different scenarios
struct client clients[] = {
    {"Uno", 3, {"Mensaje 1-1", "Mensaje 1-2", "Mensaje 1-3"}},
    {"Dos", 3, {"Mensaje 2-1", "Mensaje 2-2", "Mensaje 2-3"}}};

int main()
{
    signal(SIGUSR1, serverReady);
    int serverPID = runServer(getpid());
    while (!serverIsReady)
    {
        sleep(1);
    }
    printf("Main: Server(%d) signaled ready to receive messages\n", serverPID);

    for (int client = 0; client < NUM_CLIENTS; client++)
    {
        clients[client].PID = runClient(clients[client].name, clients[client].num_messages,
                                        clients[client].messages);
    }

    for (int client = 0; client < NUM_CLIENTS; client++)
    {
        int clientStatus;
        printf("Main: Waiting for client %s(%d) to complete...\n", clients[client].name, clients[client].PID);
        pthread_join(tidServer[client], NULL);
        waitpid(clients[client].PID, &clientStatus, 0);
        printf("Main: Client %s(%d) terminated with status: %d\n",
               clients[client].name, clients[client].PID, clientStatus);
    }

    printf("Main: Killing server (%d)\n", serverPID);
    kill(serverPID, SIGINT);
    int statusServer;
    pid_t wait_result = waitpid(serverPID, &statusServer, 0);
    printf("Main: Server(%d) terminated with status: %d\n", serverPID, statusServer);
    return 0;
}

//*********************************************************************************
//**************************** EDIT FROM HERE *************************************
//#you can create global variables and functions that you consider necessary***
//*********************************************************************************

#define PORT_NUMBER 44142

bool serverShutdown = false;

void shutdownServer(int signal)
{
    //Indicate that the server has to shutdown
    //Wait for the children to finish
    //Exit
}

void client(char *clientName, int numMessages, char *messages[])
{
    char buffer[256];
    //Open the socket
    //Connect to the server
    //For each message, write to the server and read the response
    //Accept connection and create a child proccess to read and write
    printf("Client %s(%d): Return message: %s\n", clientName, getpid(), buffer);
    fflush(stdout);
    sleep(1);
    //Close socket
}

void *pthreadServer(void *arguments)
{
    char buffer[256];
    //Read socket
    printf("Server child(%d): got message: %s\n", getpid(), buffer); //expected output
    //Write socket
}
void server()
{
    char buffer[256];
    //Handle SIGINT so the server stops when the main process kills it
    //Open the socket
    //Bind the socket
    //Signal server is ready
    //Accept connection and create a PTHREAD
    int i;
    // pthread_create(&tidServer[i], ...);

    sleep(1);
    //Close socket
}