#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 20

int count = 0;
int buffer[BUFFER_SIZE];

pthread_t *tid;

int in = 0;
int out = 0;

pthread_mutex_t mutexLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t for_produced = PTHREAD_COND_INITIALIZER;
pthread_cond_t for_consumed = PTHREAD_COND_INITIALIZER;

void insert(int item)
{
    buffer[in] = item;
    printf("in at: %d => inserted: %d | count: %d | tid: %ld\n", in, item, count, pthread_self());
}

int remove_item()
{
    int item;
    item = buffer[out];
    buffer[out] = NULL;
    count--;
    out = (out + 1) % BUFFER_SIZE;
    return item;
}

void print_b()
{
    printf("[");
    for (int i = 0; i < BUFFER_SIZE; i++)
        if (buffer[i] == NULL)
        {
            // printf(" (%d):_", i);
            printf(" _");
        }
        else
        {
            // printf(" (%d):%d", i, buffer[i]);
            printf(" %d", buffer[i]);
        }
    printf("]\n");
}

void *producer(void *param)
{
    int item;
    while (1)
    {
        pthread_mutex_lock(&mutexLock);

        // while (out == (in + 1) % BUFFER_SIZE)
        // while ((out - 1) % BUFFER_SIZE == (in + 1) % BUFFER_SIZE)
        // while ((out - 1) % BUFFER_SIZE == in)
        // while ((out - in + BUFFER_SIZE) % BUFFER_SIZE == 0)
        if ((in + 1) % BUFFER_SIZE == out)
            pthread_cond_wait(&for_consumed, &mutexLock);

        // Critical Section: Do work
        item = rand() % BUFFER_SIZE;

        insert(item);
        count++;
        in = (in + 1) % BUFFER_SIZE;

        // finish work
        print_b();
        pthread_cond_signal(&for_produced);
        pthread_mutex_unlock(&mutexLock);

        sleep(1);
    }
}
void *consumer(void *param)
{
    int item;
    while (1)
    {
        pthread_mutex_lock(&mutexLock);

        // while (out == (in + 1) % BUFFER_SIZE)
        while (out == in)
            pthread_cond_wait(&for_produced, &mutexLock);

        // Critical Section: Do work
        item = remove_item();
        printf("out at: %d => removed: %d | count: %d | tid: %ld\n", out, item, count, pthread_self());
        // finish work

        print_b();

        pthread_cond_signal(&for_consumed);
        pthread_mutex_unlock(&mutexLock);

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

    pthread_mutex_destroy(&mutexLock);
    pthread_cond_destroy(&for_produced);
    pthread_cond_destroy(&for_consumed);
}