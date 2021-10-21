#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 20

volatile int count = 0;
int buffer[BUFFER_SIZE];

pthread_t *tid;

int prod_idx = 0;
int cons_idx = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void insert(int item)
{
    buffer[prod_idx] = item;
    prod_idx = (prod_idx + 1) % BUFFER_SIZE;
    count++;
}

int remove_item()
{
    int item;
    item = buffer[cons_idx];
    cons_idx = (cons_idx + 1) % BUFFER_SIZE;
    count--;
    return item;
}

void *producer(void *param)
{
    int item;
    while (1)
    {
        pthread_mutex_lock(&mutex);

        // Used count insetead of prod_idx (in) and cons_idx (out), because
        // this leads to a bug. The buffer never gets filled to BUFFER_SIZE, because when out==in
        // regardless if it is empty, the producer cannot insert when the condition of out==in is true
        // and then it will give control away.
        //
        // Reasoning behind using count:
        // - It is protected by the mutex lock during read and write operations
        // - As long as count < BUFFER_SIZE it exists the guarantee that out <= in always.
        // - For out >= in the current count of items will need to be count > BUFFER_SIZE
        while (count == BUFFER_SIZE)
        {
            // release and acquire again the mutex to avoid starvation of the consumer
            // in case that the buffer is full
            // release lock to allow consumer to work
            pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&mutex);
        }

        // Critical Section: Do work
        item = rand() % BUFFER_SIZE;
        insert(item);
        printf("in at: %d => inserted: %d\n", prod_idx, item);
        // finish work

        // debugging output
        // printf("in at: %d => inserted: %d | count: %d | tid: %ld\n", prod_idx, item, count, pthread_self());

        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
}

void *consumer(void *param)
{
    int item;
    while (1)
    {
        pthread_mutex_lock(&mutex);

        // Used count insetead of prod_idx (in) and cons_idx (out), because
        // this leads to a bug. The buffer never gets filled to BUFFER_SIZE, because when out==in
        // regardless if it is empty, the producer cannot insert when the condition of out==in is true
        // and then it will give control away.
        while (count == 0)
        {
            // release and acquire again the mutex to avoid starvation of the producer
            // in case that the buffer is already empty
            // release lock to allow producer to work
            pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&mutex);
        }

        // Critical Section: Do work
        item = remove_item();
        printf("out at: %d => removed: %d\n", prod_idx, item);
        // finish work

        // debugging output
        // printf("out at: %d => removed: %d | count: %d | tid: %ld\n", cons_idx, item, count, pthread_self());

        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
}

int main(int argc, char *argv[])
{

    int producers = atoi(argv[1]);
    int consumers = atoi(argv[2]);
    int total_threads = producers + consumers;

    // init tid array
    tid = (pthread_t *)malloc(sizeof(pthread_t) * total_threads);

    int i;

    for (i = 0; i < producers; i++)
        pthread_create(&tid[i], NULL, producer, NULL);

    for (i = 0; i < consumers; i++)
        pthread_create(&tid[i + producers], NULL, consumer, NULL);

    for (i = 0; i < total_threads; i++)
        pthread_join(tid[i], NULL);

    free(tid);

    pthread_mutex_destroy(&mutex);
}