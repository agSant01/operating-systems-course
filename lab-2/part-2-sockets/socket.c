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
#include <wait.h>

// Prototypes for internal functions and utilities
void error(const char *fmt, ...);
int runClient(char *clientName, int numMessages, char **messages);
int runServer();
void serverReady(int signal);

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
        fflush(stderr);
        fprintf(stderr, "Main: Server(%d) launched...\n", forkPID);
    }
    return forkPID;
}

int runClient(char *clientName, int numMessages, char **messages)
{
    fflush(stderr);
    fprintf(stderr, "Launching client %s...\n", clientName);
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
    fprintf(stderr, "Main: Server(%d) signaled ready to receive messages\n", serverPID);
    fflush(stdout);

    for (int client = 0; client < NUM_CLIENTS; client++)
    {
        clients[client].PID = runClient(clients[client].name, clients[client].num_messages,
                                        clients[client].messages);
    }

    for (int client = 0; client < NUM_CLIENTS; client++)
    {
        int clientStatus;
        fprintf(stderr, "Main: Waiting for client %s(%d) to complete...\n", clients[client].name, clients[client].PID);
        waitpid(clients[client].PID, &clientStatus, 0);
        fprintf(stderr, "Main: Client %s(%d) terminated with status: %d\n",
                clients[client].name, clients[client].PID, clientStatus);
    }

    fprintf(stderr, "Main: Killing server (%d)\n", serverPID);
    kill(serverPID, SIGINT);
    int statusServer;
    pid_t wait_result = waitpid(serverPID, &statusServer, 0);
    fprintf(stderr, "Main: Server(%d) terminated with status: %d\n", serverPID, statusServer);
    return 0;
}

//*********************************************************************************
//**************************** EDIT FROM HERE *************************************
//#you can create the global variables and functions that you consider necessary***
//*********************************************************************************

#define PORT_NUMBER 17732

struct sockaddr_in SERVER_ADDR, CLIENT_ADDR;

int SERVER_SOCKET;

bool serverShutdown = false;

void shutdownServer(int signal)
{
    //Indicate that the server has to shutdown
    serverShutdown = true;

    while (wait(NULL) > 0)
    {
    }

    fprintf(stderr, "Shutting down server... | PID: %d | Signal: %d\n", getpid(), signal);
    fflush(stderr);

    close(SERVER_SOCKET);

    //Exit
    exit(0);
}

void childServerIdleTermination(int signal)
{
    fprintf(stderr, "Terminating child server PID:%d. 10 seconds of idle time...\n", getpid());
    fflush(stderr);

    // exit children
    raise(SIGINT);

    exit(0);
}

void client(char *clientName, int numMessages, char *messages[])
{
    char buffer[256];
    int socketfd, clientlen;
    struct hostent *server_host;

    // For each message, write to the server and read the response
    for (int i = 0; i < numMessages; i++)
    {
        //Open the socket
        socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketfd < 0)
            error("client: Error creating socket for client: %s", clientName);

        //Connect to the server
        server_host = gethostbyname("localhost");

        if (server_host == NULL)
            error("client: Not such host: %d", PORT_NUMBER);

        bzero((char *)&CLIENT_ADDR, sizeof(&CLIENT_ADDR));
        CLIENT_ADDR.sin_family = AF_INET;
        bcopy((char *)server_host->h_addr,
              (char *)&CLIENT_ADDR.sin_addr.s_addr,
              server_host->h_length);
        CLIENT_ADDR.sin_port = htons(PORT_NUMBER);

        // Accept connection and create a child proccess to read and write
        clientlen = sizeof(CLIENT_ADDR);

        if (connect(socketfd, (struct sockaddr *)&CLIENT_ADDR, clientlen) < 0)
            error("Error connecting to server...");

        pid_t pid = fork();

        if (pid < 0)
            error("Error creating client subprocess...");
        else if (pid == 0)
        {
            // child
            if (write(socketfd, messages[i], strlen(messages[i])) < 0)
                error("Client %s(%d): Error on writing to socket: message: %s\n", clientName, getpid(), buffer);

            bzero(buffer, 256);
            if (read(socketfd, &buffer, 255) < 0)
                error("Client %s(%d): Error on reading from socket: message %s\n", clientName, getpid(), buffer);

            // verbose:
            // printf("Client %s(%d): Return message: %s\n", clientName, getpid(), buffer);

            printf("%s : %d : %s\n", clientName, getppid(), buffer);
            fflush(stdout);

            exit(0); // exit read/write child
        }

        sleep(1);
    }

    //Close socket
    close(socketfd);
}

void server()
{
    char buffer[256];
    int newsocketfd, clientlen;

    //Handle SIGINT so the server stops when the main process kills it
    signal(SIGINT, shutdownServer);

    // Handle SIGALRM, so the children processes exit after 10 seconds of idle time
    signal(SIGALRM, childServerIdleTermination);

    //Open the socket
    SERVER_SOCKET = socket(AF_INET, SOCK_STREAM, 0);

    if (SERVER_SOCKET < 0)
        error("server: Error creating SERVER socket.");

    // clear socket address space
    bzero((char *)&SERVER_ADDR, sizeof(SERVER_ADDR));
    SERVER_ADDR.sin_family = AF_INET;
    SERVER_ADDR.sin_addr.s_addr = INADDR_ANY;
    SERVER_ADDR.sin_port = htons(PORT_NUMBER);

    // Bind the socket
    if (bind(SERVER_SOCKET, (struct sockaddr *)&SERVER_ADDR, sizeof(SERVER_ADDR)) < 0)
        error("server: Error binding socket...");

    // Accept connection and create a child proccess to read and write
    listen(SERVER_SOCKET, MAX_MESSAGES);

    // Send Signal server is ready
    if (kill(getppid(), SIGUSR1) < 0)
        error("server: Error signaling MAIN that server is ready...");

    clientlen = sizeof(CLIENT_ADDR); // precompute size

    pid_t childpid = getpid(); // initialize with parent (current) pid

    // newsocketfd es el mailbox
    while (!serverShutdown)
    {
        newsocketfd = accept(SERVER_SOCKET, (struct sockaddr *)&CLIENT_ADDR, (socklen_t *)&clientlen); // blocking call

        if (newsocketfd < 0)
            error("server: Error on accept...");

        // if the current process is a child
        // do not create a sub-child
        // childs can't create more childs
        if (childpid != 0)
        {
            // first iteration is not a child
            // create a child process
            //  - for child childpid = 0;
            //  - for parent childpid = <positive integer>;
            childpid = fork();
        }

        if (childpid < 0)
            // error on fork()
            error("server: Error creating child process for request on socket: %d...", newsocketfd);
        else if (childpid == 0)
        {
            signal(SIGINT, NULL); // remove signal of server shutdown for the child

            // child process just to manage connection,
            // dispose after with exit(0);
            bzero(buffer, 256);                      // clear buffer
            if (read(newsocketfd, &buffer, 255) < 0) // read complete buffer
                error("server: Error reading from socket: Mailbox Socket (%d): Server child(%d)",
                      newsocketfd, getpid());

            // verbose:
            // printf("Server child(%d): got message: %s\n", getpid(), buffer); //expected output
            // fflush(stdout);
            printf("Server : %d : %s\n", getpid(), buffer);
            fflush(stdout);

            // verbose:
            // sprintf(buffer, "I (serverChildPid: %d) got your message", getpid());

            if (write(newsocketfd, &buffer, strlen(buffer)) < 0)
                error("Server child(%d): Error writing response to client");

            close(newsocketfd);

            // set SIGALRM for end of child life-span
            // if child spends more than 10 seconds without receiving any connection/request: kill process
            // Docs: https://linux.die.net/man/3/alarm
            //  Alarm requests are not stacked; only one SIGALRM generation can be scheduled in this manner.
            //  If the SIGALRM signal has not yet been generated, the call shall result in rescheduling the
            //  time at which the SIGALRM signal is generated.
            alarm(10);
        }

        // Do nothing on parent
        // Parent just spawns children processes
    }
}