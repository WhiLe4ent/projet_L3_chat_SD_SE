#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080

// Function to send messages to the server
void *send_message(void *arg) {
    int sock = *((int *)arg);
    char message[1024];

    while (1) {
        
        // Read message from user input
        printf("Enter a message: ");
        fgets(message, sizeof(message), stdin);

        // Send message to the server
        send(sock, message, strlen(message), 0);
    }

    return NULL;
}

int main(int argc, char const *argv[]) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Create client socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    // Create a thread to send messages to the server
    pthread_t send_thread;
    if (pthread_create(&send_thread, NULL, send_message, (void *)&sock) != 0) {
        printf("Failed to create send thread\n");
        return -1;
    }

    // Receive and print messages from the server
    while (1) {
        memset(buffer, 0, 1024);

        // Read message from the server
        valread = read(sock, buffer, 1024);
        if (valread <= 0) {
            printf("\nServer disconnected\n");
            break;
        }
        printf("Server: %s", buffer);
    }

    // Close the socket
    close(sock);

    return 0;
}
