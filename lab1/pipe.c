#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	// Checking Arguments
    if (argc < 2) {
        errno = EINVAL;
        perror("Usage: command1, command2");
        exit(EXIT_FAILURE);
    }

    // Creating Pipes
    int pipes[argc - 1][2]; // Adjust the size of the pipes array

    // Create pipes
    for (int i = 0; i < argc - 2; i++) { // You still create argc-2 pipes
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < argc - 1; i++) { // Adjust your loop to also fork for the last command
        pid_t p = fork();

        // Forking Failure Check
        if (p == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        // Child Process
        if (p == 0) {
            // If it's not the first command, redirect standard input from the previous pipe's read end
            if (i != 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][1]);
            }

            // If it's not the last command, redirect standard output to the next pipe's write end
            if (i < argc - 2) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipes in the child, we've already duplicated the ones we needed
            for (int j = 0; j < argc - 2; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            execlp(argv[i + 1], argv[i + 1], (char *)NULL);
            // If execlp returns, it's an error
            perror("execlp");
            exit(EXIT_FAILURE);
        }
    }

    // In the parent: close all pipes, they're not needed anymore
    for (int i = 0; i < argc - 2; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < argc - 1; i++) {
        int status;
        if (wait(&status) == -1) {
            perror("wait");
            exit(EXIT_FAILURE);
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Error: Child process %d exited with error code %d.\n", i, WEXITSTATUS(status));
            exit(EXIT_FAILURE);
        }
    }
	return 0;
}
