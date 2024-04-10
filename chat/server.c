#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 10 // Maximum number of clients

// Structure to hold client information
typedef struct {
    int id;
    int socket;
} Client;

// Global array to store connected clients
Client clients[MAX_CLIENTS];
int num_clients = 0; // Number of connected clients

// Function to handle each client connection
void *handle_client(void *arg) {
    int client_index = *((int*)arg);
    int client_socket = clients[client_index].socket;
    int client_id = clients[client_index].id;

    char buffer[2048] = {0};

    while (1) {
        // Read data from client
        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_socket, buffer, sizeof(buffer));
        if (valread <= 0) {
            // Client disconnected or error occurred
            printf("Client %d disconnected\n", client_id);
            close(client_socket);
            pthread_exit(NULL);
        }
        printf("Message from client %d: %s\n", client_id, buffer);

        // Send the message to all clients, including the one who sent it
        for (int i = 0; i < num_clients; i++) {
            // +1 for null terminator
            int required_size = snprintf(NULL, 0, "client %d: %s", client_id, buffer) + 1; 
            char *message = (char *)malloc(required_size);

            if (message == NULL) {
                perror("malloc failed");
                exit(EXIT_FAILURE);
            }
            snprintf(message, required_size, "client %d: %s", client_id, buffer);

            
            send(clients[i].socket, message, strlen(message), 0);

            // free messgae we don't need it anymore
            free(message);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
         // Accept incoming connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Receive client ID
        int client_id;
        if (recv(new_socket, &client_id, sizeof(int), 0) <= 0) {
            perror("recv failed");
            close(new_socket);
            continue; // Continue to next iteration
        }

        // Store the client information
        clients[num_clients].id = client_id;
        clients[num_clients].socket = new_socket;
        num_clients++;


        // Create a new thread to handle the client
        pthread_t client_thread;
        int *client_index = malloc(sizeof(int));
        *client_index = num_clients - 1; // Pass the index of the client in the array
        if (pthread_create(&client_thread, NULL, handle_client, (void *)client_index) < 0) {
            perror("could not create thread");
            free(client_index);
            close(new_socket);
            exit(EXIT_FAILURE);
        }

        // Detach the thread
        pthread_detach(client_thread);

        printf("Client %d connected\n", client_id);
    }

    return 0;
}