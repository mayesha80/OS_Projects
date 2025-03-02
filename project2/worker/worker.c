#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define MAX_THREAD 10

typedef struct {
    int *data;
} __thread_data;

int total;
__thread_data* thread_data;
int thread_states;  // 1: running, 0: terminating
pthread_mutex_t lock;  // Mutex lock to protect shared data

void signalHandler(int sig) {
    int i;

    printf("Main: caught SIGINT (Ctrl+C). Exiting gracefully.\n");

    // Set thread state to terminating
    thread_states = 0;

    // Wait for all threads to complete before accessing shared data
    for (i = 0; i < MAX_THREAD; i++) {
        pthread_mutex_lock(&lock);
        total += thread_data[i].data[0];
        total += thread_data[i].data[1];
        total += thread_data[i].data[2];
        pthread_mutex_unlock(&lock);

        free(thread_data[i].data);
    }

    free(thread_data);
}

void* worker(void* arg) {
    long tid = (long)arg;

    srand(time(NULL) * tid);
    pthread_mutex_lock(&lock);
    thread_data[tid].data[0] = 0;
    thread_data[tid].data[1] = 1;
    thread_data[tid].data[2] = 1;
    pthread_mutex_unlock(&lock);

    while (1) {
        if (thread_states == 0) {
            printf("Thread #%ld: terminating..\n", tid);
            return NULL;
        }

        pthread_mutex_lock(&lock);
        thread_data[tid].data[0] += rand() % 10;
        thread_data[tid].data[1] += rand() % 10;
        thread_data[tid].data[2] += rand() % 10;
        pthread_mutex_unlock(&lock);

        usleep(1000);
    }
    return NULL;
}

int main() {
    long i;  // Declare 'i' here to use consistently in the main function
    pthread_t thread[MAX_THREAD];

    if (signal(SIGINT, signalHandler) == SIG_ERR) {
        printf("Failed to register a signal handler.\n");
        return 0;
    }

    total = 0;
    thread_data = malloc(sizeof(__thread_data) * MAX_THREAD);
    thread_states = 1;

    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    for (i = 0; i < MAX_THREAD; i++) {
        thread_data[i].data = (int*)malloc(sizeof(int) * 3);
        pthread_create(&thread[i], NULL, worker, (void*)i);
    }

    for (i = 0; i < MAX_THREAD; i++) {
        pthread_join(thread[i], NULL);
    }

    // Destroy the mutex
    pthread_mutex_destroy(&lock);

    printf("total = %d\n", total);
    return 0;
}