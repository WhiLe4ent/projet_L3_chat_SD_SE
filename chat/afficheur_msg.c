#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    FILE *pipe_fp;
    char pipe_buffer[1024];

    // Open the pipe for reading
    pipe_fp = popen("./client", "r");
    if (pipe_fp == NULL) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    // Read from the pipe and display messages
    while (fgets(pipe_buffer, sizeof(pipe_buffer), pipe_fp) != NULL) {
        printf("Received from server: %s", pipe_buffer);
    }

    // Close the pipe
    pclose(pipe_fp);

    return 0;
}
