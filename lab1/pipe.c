#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	if (argc < 2) {
        errno = EINVAL;
        perror("Error");
        exit(errno);
    }

    int fd[2];
    pid_t pid;
    int in_fd = STDIN_FILENO;  // Starts with standard input

    for (int i = 1; i < argc; i++) {
        if (i != argc - 1 && pipe(fd) < 0) {
            perror("Pipe creation error");
            exit(errno);
        }

        if ((pid = fork()) < 0) {
            perror("Fork error");
            exit(errno);
        }

        if (pid == 0) { // child
            if (i != 1) {
                dup2(in_fd, STDIN_FILENO);  // If not the first command, read from previous pipe
            }
            if (i != argc - 1) {
                dup2(fd[1], STDOUT_FILENO);  // If not the last command, write to next pipe
                close(fd[0]);  // Close read side as it's not used
            }
            close(in_fd);  // Close this as we've duplicated the descriptor we needed
            execlp(argv[i], argv[i], (char *)NULL);
            perror("Exec error");
            exit(errno);
        } else {
            close(fd[1]);  // Close write side
            if (i != 1) {
                close(in_fd);  // Close the previous read descriptor
            }
            in_fd = fd[0];  // Update in_fd to current read end for next iteration
        }
    }

    // Wait for all children to finish
    while (wait(NULL) > 0);
	return 0;
}
