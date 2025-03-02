#include "stringbuffer.hpp"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


StringBuffer *buffer = new StringBuffer("abc");
pthread_rwlock_t rw_lock;

void *thread1(void *args) {
    int n = 0;

    while (1) {
        pthread_rwlock_wrlock(&rw_lock);  // Acquire write lock
        buffer->erase(0, 3);
        buffer->append("abc");
        pthread_rwlock_unlock(&rw_lock);  // Release write lock
        if (n++ % 100 == 0) printf("%dth try - Thread 1: erase and append: %d\n", n - 1);
        usleep(100);
    }
}

void *thread2(void *args) {
    int n = 0;

    while (1) {
        StringBuffer *sb = new StringBuffer();
        pthread_rwlock_rdlock(&rw_lock);  // Acquire read lock
        sb->append(buffer);
        pthread_rwlock_unlock(&rw_lock);  // Release read lock
        usleep(100);
        if (n++ % 100 == 0) printf("%dth try - Thread 2: create and append: %d\n", n - 1);
        delete sb;
    }
}

int main(int argc, char *argv[]) {
    pthread_t th1, th2;

    // Initialize the read-write lock
    pthread_rwlock_init(&rw_lock, NULL);

    pthread_create(&th1, NULL, thread1, NULL);
    pthread_create(&th2, NULL, thread2, NULL);

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    // Destroy the read-write lock
    pthread_rwlock_destroy(&rw_lock);

    return 0;
}