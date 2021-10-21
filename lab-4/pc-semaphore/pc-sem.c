#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 20

int buffer[BUFFER_SIZE];

pthread_t *tid;

int count = 0; // left it for debugging purposes
int prod_idx = 0;
int cons_idx = 0;

sem_t empty_spaces;
sem_t full_spaces;

sem_t sharedLock;

void insert(int item)
{
    buffer[prod_idx] = item;
    count++;
    // debugging purpose
    prod_idx = (prod_idx + 1) % BUFFER_SIZE;
}

int remove_item()
{
    int item;

    item = buffer[cons_idx];
    buffer[cons_idx] = NULL;
    count--; // debugging purpose
    cons_idx = (cons_idx + 1) % BUFFER_SIZE;

    return item;
}
void *producer(void *param)
{
    int item;
    while (1)
    {
        sem_wait(&empty_spaces);
        sem_wait(&sharedLock);

        // Critical Section: Do work
        item = rand() % BUFFER_SIZE;
        insert(item);
        printf("in at: %d => inserted: %d\n", prod_idx, item);
        // finish work

        // debugging output
        // printf("in at: %d => inserted: %d | count: %d | tid: %ld\n", prod_idx, item, count, pthread_self());

        sem_post(&sharedLock);
        sem_post(&full_spaces);

        sleep(1);
    }
}
void *consumer(void *param)
{
    int item;
    while (1)
    {
        sem_wait(&full_spaces);
        sem_wait(&sharedLock);

        // Critical Section: Do work
        item = remove_item();
        printf("out at: %d => removed: %d\n", prod_idx, item);
        // finish work

        // debugging output
        // printf("out at: %d => removed: %d | count: %d | tid: %ld\n", cons_idx, item, count, pthread_self());

        sem_post(&sharedLock);
        sem_post(&empty_spaces);

        sleep(1);
    }
}
int main(int argc, char *argv[])
{
    int producers = atoi(argv[1]);
    int consumers = atoi(argv[2]);

    int total_threads = producers + consumers;

    tid = (pthread_t *)malloc(sizeof(pthread_t) * total_threads);

    sem_init(&empty_spaces, 0, BUFFER_SIZE);
    sem_init(&full_spaces, 0, 0);
    sem_init(&sharedLock, 0, 1);

    int i;
    for (i = 0; i < producers; i++)
        pthread_create(&tid[i], NULL, producer, NULL);

    for (i = 0; i < consumers; i++)
        pthread_create(&tid[i + producers], NULL, consumer, NULL);

    for (i = 0; i < total_threads; i++)
        pthread_join(tid[i], NULL);

    free(tid);

    sem_destroy(&empty_spaces);
    sem_destroy(&full_spaces);
    sem_destroy(&sharedLock);
}