#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PIPE_NAME "message_pipe"

int main() {
    int pipe_fd;
    char pipe_buffer[1024];
    // int client_id = -1; // Default client ID

    // Open the pipe for reading
    pipe_fd = open(PIPE_NAME, O_RDONLY);
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    // Indicate connection in the terminal
    printf("Afficheur message connected.\n");

    // Read from the pipe and display messages
    while (read(pipe_fd, pipe_buffer, sizeof(pipe_buffer)) != 0) {
        // Print client ID and message
        printf("%s", pipe_buffer);
    }

    // Close the pipe
    close(pipe_fd);

    return 0;
}