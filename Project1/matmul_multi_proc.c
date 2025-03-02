#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // For memset
#include "matmul.h"

#define MAX_RETRIES 3  // Maximum retries

typedef struct plist_t {
    int pid;
    int row_start;
    int row_end;
    int retries;
} plist_t;

// Circular queue implementation start
typedef struct queue_t {
    plist_t *arr;
    int size, head, tail, cur_size;
} queue_t;

// Queue functions
void init(queue_t *q, int size) {
    q->size = size;
    q->arr = malloc(size * sizeof(plist_t));
    q->head = q->tail = 0;
    q->cur_size = 0;
}

void add(queue_t *q, plist_t p) {
    if (q->cur_size == q->size) {
        printf("Queue is full\n");
        return;
    }
    q->arr[q->tail] = p;
    q->tail = (q->tail + 1) % q->size;
    ++(q->cur_size);
}

int pop(queue_t *q) {
    if (q->cur_size == 0) {
        printf("Queue is empty\n");
        return -1;
    }
    --(q->cur_size);
    int ptr = q->head;
    q->head = (q->head + 1) % q->size;
    return ptr;
}

// Simulate crash for multi-process
int should_crash_process(int crashRate) {
    if (crashRate == 0) return 0;
    srand(getpid());
    int randomValue = rand() % 100;
    return (randomValue < crashRate);
}

// Child process function to compute part of the matrix and send results via pipe
void child_process_core(int row_start, int row_end, int pipe_fd[2], int crashRate) {
    close(pipe_fd[0]);  // Close read end of the pipe for the child

    // Simulate a crash based on the crash rate
    if (should_crash_process(crashRate)) {
        printf("Child process %d crashed\n", getpid());
        close(pipe_fd[1]);  // Close the write end before crashing
        abort();  // Simulate crash by abnormal termination
    }

    // Matrix multiplication logic for assigned rows
    int result[row_end - row_start][p];  // Temporary buffer to store the result of this process
    memset(result, 0, sizeof(result));  // Initialize result to 0

    int i, j, k;
    for (i = 0; i < (row_end - row_start); i++) {
        for (j = 0; j < p; j++) {
            for (k = 0; k < n; k++) {
                result[i][j] += A[row_start + i][k] * B_tran[j][k];
            }
        }
    }

    // Write the result of this process to the pipe
    write(pipe_fd[1], result, (row_end - row_start) * sizeof(int) * p);

    close(pipe_fd[1]);  // Close write end of the pipe
    exit(0);  // Exit normally
}

void mat_mult_multi_proc(int numProc) {
    int pipe_fd[numProc][2];  // Array of pipes for communication
    pid_t pids[numProc];
    queue_t q;
    init(&q, numProc);  // Initialize the process queue
    
    int rows_per_proc = m / numProc;
    int remaining_rows = m % numProc;
    int i, j;

    // Fork child processes and handle crashes
    for (i = 0; i < numProc; i++) {
        int row_start = i * rows_per_proc;
        int row_end = (i == numProc - 1) ? (row_start + rows_per_proc + remaining_rows) : (row_start + rows_per_proc);

        plist_t proc_info;
        proc_info.row_start = row_start;
        proc_info.row_end = row_end;
        proc_info.retries = 0; 

        if (pipe(pipe_fd[i]) == -1) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }

        pid_t pid;
        do {
            pid = fork();
        } while (pid < 0);

        if (pid == 0) {
            // Child process: execute matrix multiplication and potentially crash
            close(pipe_fd[i][0]);  // Close read end of pipe in the child
            child_process_core(row_start, row_end, pipe_fd[i], crashRate);  // Use the global crashRate
        } else if (pid > 0) {
            // Parent process: store process info and add to queue
            close(pipe_fd[i][1]);  // Close write end of pipe in the parent
            proc_info.pid = pid;
            add(&q, proc_info);  // Add process info to queue
        }
    }

    while (q.cur_size > 0) {
        int id = pop(&q);  // Pull from the queue
        int status;
        plist_t proc_info = q.arr[id];
        waitpid(proc_info.pid, &status, 0);  // Wait for the child process to finish

        if (WIFEXITED(status)) {  // Normal termination
            printf("Process %d completed successfully. Exit code: %d\n", proc_info.pid, WEXITSTATUS(status));

            // Parent reads the result from the pipe
            int rows_to_read = proc_info.row_end - proc_info.row_start;
            for (i = 0; i < rows_to_read; i++) {
                read(pipe_fd[id][0], C_multi_proc[proc_info.row_start + i], p * sizeof(int));
            }
            close(pipe_fd[id][0]);  // Close read end of the pipe after reading

        } else if (WIFSIGNALED(status)) {  // Abnormal termination (crash)
            printf("Process %d terminated by signal %d\n", proc_info.pid, WTERMSIG(status));

            // Retry crashed process if retries are below MAX_RETRIES
            if (proc_info.retries < MAX_RETRIES) {
                if (pipe(pipe_fd[id]) == -1) {  // Create a new pipe for the retry
                    perror("pipe failed");
                    exit(EXIT_FAILURE);
                }

                pid_t new_pid = fork();
pid_t old_pid = proc_info.pid;  // Capture the old (crashed) PID before updating
if (new_pid == 0) {
    // New child process replaces the crashed one
    close(pipe_fd[id][0]);  // Close the read end in the new child process
    child_process_core(proc_info.row_start, proc_info.row_end, pipe_fd[id], crashRate);
} else if (new_pid > 0) {
    close(pipe_fd[id][1]);  // Close the write end in the parent
    proc_info.pid = new_pid;
    proc_info.retries++;
    // Corrected: remove the 'pid_t' from the printf statement
    printf("Retrying process %d with new process %d (attempt %d)\n", old_pid, new_pid, proc_info.retries);
    add(&q, proc_info);  // Add to the queue for monitoring
}
 // Add to the queue for monitoring
                else {
                    perror("fork failed");
                    exit(EXIT_FAILURE);
                }
            } else {
                printf("Process handling rows starting at %d has reached the retry limit. No further attempts will be made.\n", proc_info.row_start);
                // Here you might want to implement a fallback calculation for these rows
            }
        }
    }
}
