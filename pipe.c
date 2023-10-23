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

    // Iterate through the arguments (processes to run)
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
            if (i != argc - 1) {
                dup2(fd[1], STDOUT_FILENO);  // Redirect stdout to pipe's write
                close(fd[0]);
                close(fd[1]);
            }
            execlp(argv[i], argv[i], (char *)NULL);
            perror("Exec error");
            exit(errno);
        } else { // parent
            if (i != argc - 1) {
                dup2(fd[0], STDIN_FILENO);   // Redirect stdin to pipe's read
                close(fd[0]);
                close(fd[1]);
            }
        }
    }

    // parent waits for all children to finish
    for (int i = 1; i < argc; i++) {
        wait(NULL);
    }
	
	return 0;
}
