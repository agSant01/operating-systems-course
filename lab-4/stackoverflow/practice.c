#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

int SHARED_VAR = 0;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

int counter = 0;

void *runner(void *param)
{
    pthread_mutex_lock(&mtx);

    while (counter < 10)
    {
        // do action
        pthread_cond_wait(&cv, &mtx);

        counter++;
        fflush(stdout);
        printf("Modified by thread: %d | Counter: %d", param, counter);
        fflush(stdout);
    }

    // state is false
    fflush(stdout);
    printf("Exit thread: %d | Counter: %d", param, counter);
    fflush(stdout);

    pthread_mutex_unlock(&mtx);

    pthread_exit(0);
}

void main()
{
    pthread_t tid[2];

    pthread_attr_t attr;

    for (int i = 0; i < 2; i++)
        pthread_attr_init(&attr);

    pthread_create(&tid[0], &attr, runner, 1);
    pthread_create(&tid[1], &attr, runner, 2);

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
}