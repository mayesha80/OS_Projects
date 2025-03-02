#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h> 

#define MAX_RETRIES 3  // Maximum retries for each thread

typedef struct tlist_t {
    pthread_t tid;
    int row_start;
    int row_end;
    int retries;
    int failed;
    int crashRate;   // Track if this thread has already failed
} tlist_t;

// Matrix dimensions and matrices
int **A, **B_tran, **C_multi_thread;
int m, n, p;  // m: rows of A, n: cols of A, p: cols of B

int *completed_rows;  // Array to track which rows have been completed

// Circular queue implementation
typedef struct queue_t {
    tlist_t *arr;
    int size, head, tail, cur_size;
} queue_t;

// Mark functions as `static` to limit scope to this file
static void init(queue_t *q, int size) {
    q->size = size;
    q->arr = malloc(size * sizeof(tlist_t));
    q->head = q->tail = 0;
    q->cur_size = 0;
}

static void add(queue_t *q, tlist_t t) {
    if (q->cur_size == q->size) {
        printf("Queue is full\n");
        return;
    }
    q->arr[q->tail] = t;
    q->tail = (q->tail + 1) % q->size;
    ++(q->cur_size);
}

static int pop(queue_t *q) {
    if (q->cur_size == 0) {
        printf("Queue is empty\n");
        return -1;
    }
    --(q->cur_size);
    int ptr = q->head;
    q->head = (q->head + 1) % q->size;
    return ptr;
}

// Generate a more varied seed for randomness using thread ID and static counter
unsigned int generate_seed() {
    static int counter = 0;
    uintptr_t thread_id = (uintptr_t) pthread_self();
    uintptr_t address = (uintptr_t) &counter;
    unsigned int seed = (unsigned int) (thread_id ^ address);
    counter++;
    return seed;
}

int should_crash_thread(int crashRate) {
    if (crashRate == 0) {
        return 0;
    }

    int randomValue = rand() % 100; 
    return (randomValue < crashRate); // If random number is less than crash rate, simulate a crash
}

// Thread function to compute part of the matrix
void* thread_core(void* arg) {
    tlist_t *data = (tlist_t*)arg;

    // Simulate crash based on the crash rate
    if (should_crash_thread(data->crashRate)) {
        printf("Thread %ld crashed\n", pthread_self());
        pthread_exit(NULL);  // Simulate thread crash
    }

    // Matrix multiplication logic for assigned rows
    int i, j, k;
    
    for (i = data->row_start; i < data->row_end; i++) {
        if (completed_rows[i]) {
            continue;  // Skip already completed rows
        }

        for (j = 0; j < p; j++) {
            C_multi_thread[i][j] = 0;
            for (k = 0; k < n; k++) {
                C_multi_thread[i][j] += A[i][k] * B_tran[j][k];
            }
        }

        completed_rows[i] = 1;  // Mark this row as completed
    }

    // Properly return an integer exit code
    int *exit_code = malloc(sizeof(int));  // Allocate memory for the exit code
    *exit_code = 0;  // Set exit code to 0 (success)
    pthread_exit((void*)exit_code);  // Exit normally with exit code
}

void mat_mult_multi_thread(int numThread, int crashRate) {
    queue_t *q;
    q = malloc(sizeof(queue_t));
    init(q, numThread);

    pthread_t tids[numThread];
    tlist_t thread_data[numThread];
    int rows_per_thread = m / numThread;
    int remaining_rows = m % numThread;
    int i;

    completed_rows = (int*)calloc(m, sizeof(int));  // Initialize completed_rows to 0 (none completed)

    // Create threads and assign work
    for (i = 0; i < numThread; i++) {
        int row_start = i * rows_per_thread;
        int row_end = (i == numThread - 1) ? (row_start + rows_per_thread + remaining_rows) : (row_start + rows_per_thread);
        thread_data[i].row_start = row_start;
        thread_data[i].row_end = row_end;
        thread_data[i].retries = 0;
        thread_data[i].failed = 0;
        thread_data[i].crashRate = crashRate;  // Set crashRate here

        // Retry creating the thread until it succeeds
        int create_status;
        do {
            create_status = pthread_create(&tids[i], NULL, thread_core, &thread_data[i]);
        } while (create_status != 0);

        if (create_status == 0) {
            thread_data[i].tid = tids[i];
            add(q, thread_data[i]);  // Add thread data to queue for monitoring
        } else {
            printf("Thread %d creation failed.\n", i);
        }
    }

    // Monitor threads for completion or crashes, respawn crashed threads
    while (q->cur_size > 0) {
        int id = pop(q);  // Pull from the queue
        void *status;

        tlist_t t = q->arr[id];  // Get thread information

        // Wait for the thread to finish
        if (pthread_join(t.tid, &status) != 0 || status == NULL) {
            // Thread crashed or terminated abnormally
            printf("Thread %ld terminated. Retrying... \n", t.tid);

            // Retry the crashed thread
            if (t.retries < MAX_RETRIES) {
                t.retries++;
                t.failed = 1;  // Mark thread as failed

                // Reset completed rows for the range this thread was handling
                for (i = t.row_start; i < t.row_end; i++) {
                    completed_rows[i] = 0;  // Mark rows as incomplete
                }

                // Try to respawn the thread, retrying if creation fails
                int create_status;
                do {
                    create_status = pthread_create(&t.tid, NULL, thread_core, &t);
                } while (create_status != 0);

                printf("Retrying thread %lu with new thread (attempt %d)\n", t.tid, t.retries);

                // Add the respawned thread back to the queue for monitoring
                add(q, t);
            } else {
                // Maximum retries reached, give up on this thread
                printf("Thread %ld failed after maximum retries. Reassigning uncompleted rows.\n", t.tid);

            
                // Reassign uncompleted rows to another thread
                for (i = t.row_start; i < t.row_end; i++) {
                    if (!completed_rows[i]) {
                        printf("Reassigning row %d to another thread.\n", i);
                        // Create a new thread to handle uncompleted rows
                        tlist_t new_thread_data;
                        new_thread_data.row_start = i;
                        new_thread_data.row_end = i + 1;
                        new_thread_data.retries = 0;
                        new_thread_data.failed = 0;

                        int create_status;
                        do {
                            create_status = pthread_create(&new_thread_data.tid, NULL, thread_core, &new_thread_data);
                        } while (create_status != 0);

                        if (create_status == 0) {
                            add(q, new_thread_data);  // Add new thread data to queue for monitoring
                        }
                    }
                }
            }
        } else {
            // Normal thread termination
            if (status != NULL) {
                int *exit_code = (int*)status;
                printf("Thread %ld completed successfully. Exit code: %d\n", t.tid, *exit_code);
                free(exit_code);  // Free the dynamically allocated exit code
            }
        }
    }

        
    

    // Ensure all rows are completed at the end
 // Example check for uncomputed rows
for (i = 0; i < m; i++) {
    if (!completed_rows[i]) {
        printf("Final attempt to calculate row %d.\n", i);
        // Create a new thread to handle the uncompleted row
        tlist_t final_thread_data;
        final_thread_data.row_start = i;
        final_thread_data.row_end = i + 1;
        final_thread_data.retries = 0;
        final_thread_data.failed = 0;

        int create_status;
        do {
            create_status = pthread_create(&final_thread_data.tid, NULL, thread_core, &final_thread_data);
        } while (create_status != 0);

        pthread_join(final_thread_data.tid, NULL);  // Ensure this row gets completed
    }
}


    // Clean up
    free(q->arr);
    free(q);
    free(completed_rows);  // Free progress tracker
}
