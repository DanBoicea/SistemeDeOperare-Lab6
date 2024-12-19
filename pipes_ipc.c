#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define NUM_PROCESSES 10

// Function to check if a number is prime
bool is_prime(int num) {
    if (num <= 1) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

// Function to find primes in a given range [start, end]
void find_primes(int start, int end, int *pipe_fd) {
    for (int i = start; i <= end; ++i) {
        if (is_prime(i)) {
            write(pipe_fd[1], &i, sizeof(i));  // Send prime number to the pipe
        }
    }
    close(pipe_fd[1]);  // Close the write end of the pipe
}

int main() {
    // Create pipes for communication between parent and child processes
    int pipes[NUM_PROCESSES][2];  // Each process gets a pipe (read and write)

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe creation failed");
            return 1;
        }
    }

    // Create 10 child processes
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        pid_t pid = fork();

        if (pid == 0) {
            // In the child process:
            // Close all unnecessary file descriptors
            for (int j = 0; j < NUM_PROCESSES; ++j) {
                if (j != i) {
                    close(pipes[j][0]);  // Close the read end of all pipes not assigned to this child
                    close(pipes[j][1]);  // Close the write end of all pipes not assigned to this child
                }
            }

            // Close the read end of the current pipe (child only writes)
            close(pipes[i][0]);

            // Assign a range to each process
            int start = i * 1000 + 1;
            int end = (i + 1) * 1000;

            // Find primes in the assigned range and send them through the pipe
            find_primes(start, end, pipes[i]);

            // Close the write end of the pipe after writing
            close(pipes[i][1]);

            // Exit the child process
            exit(0);
        } else if (pid < 0) {
            perror("Fork failed");
            return 1;
        }
    }

    // Parent process: Collect and display primes from all child processes
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        // Close all unnecessary file descriptors in the parent
        close(pipes[i][1]);  // Close the write end of the pipe for this process

        int prime;
        printf("Received from child process %d [%d - %d]: ", i + 1, i * 1000 + 1, (i + 1) * 1000);

        // Read and display primes from the pipe
        while (read(pipes[i][0], &prime, sizeof(prime)) > 0) {
            printf("%d ", prime);
        }
        printf("\n");

        close(pipes[i][0]);  // Close the read end of the pipe after reading
    }

    // Wait for all child processes to finish
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        wait(NULL);
    }

    return 0;
}

