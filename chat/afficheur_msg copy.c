#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>

#define PIPE_NAME "message_pipe"

int main() {
    int pipe_fd;
    char pipe_buffer[1024];

    // Open the pipe for reading
    pipe_fd = open(PIPE_NAME, O_RDONLY);
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    // File descriptor set for select
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(pipe_fd, &read_fds);

    while (1) {
        // Wait for data to be available in the pipe
        if (select(pipe_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check if data is available to read
        if (FD_ISSET(pipe_fd, &read_fds)) {
            // Read from the pipe and display messages
            ssize_t bytes_read = read(pipe_fd, pipe_buffer, sizeof(pipe_buffer));
            if (bytes_read == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            } else if (bytes_read == 0) {
                // Pipe has been closed
                break;
            } else {
                printf("Received from client: %.*s", (int)bytes_read, pipe_buffer);
            }
        }
    }

    // Close the pipe
    close(pipe_fd);

    return 0;
}
