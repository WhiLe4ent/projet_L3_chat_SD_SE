#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h> // Pour les constantes de mode d'ouverture des pipes
#include <sys/stat.h> // Pour la fonction mkfifo

#define PORT 8080
#define MAX_CLIENTS 10 // Maximum number of clients
#define MAX_ID_LENGTH 50 // Maximum length of client ID

// Déclarer le pipe nommé comme une variable globale
char *pipe_name = "../file_com";

// Structure to hold client information
typedef struct {
    char id[MAX_ID_LENGTH];
    pthread_t thread_id;
    int socket;
} Client;

// Global array to store connected clients
Client clients[MAX_CLIENTS];
int num_clients = 0; // Number of connected clients

// Function to handle each client connection
void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char client_id[MAX_ID_LENGTH];
    int index = -1;

    char buffer[2048 + 5] = {0};

    // Open the named pipe for reading and writing
    int pipe_fd = open(pipe_name, O_RDWR);
    if (pipe_fd == -1) {
        perror("open");
        close(client_socket);
        pthread_exit(NULL);
    }

    pthread_t thread_id = pthread_self(); // Get the ID of the current thread

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // Read data from client
        int valread = read(client_socket, buffer, sizeof(buffer));

        if (valread <= 0) {
            // Client disconnected or error occurred
            printf("Client %s disconnected\n", client_id);
            close(client_socket);
            if (index != -1) {
                // Remove client from the array
                for (int i = index; i < num_clients - 1; i++) {
                    strcpy(clients[i].id, clients[i + 1].id);
                    clients[i].socket = clients[i + 1].socket;
                }
                num_clients--;
            }
            close(pipe_fd); // Close the pipe
            pthread_exit(NULL);
        }

        // // Send the received message to the named pipe along with the thread ID
        // char message_with_thread_id[2053]; // Augmenter la taille si nécessaire
        // snprintf(message_with_thread_id, sizeof(message_with_thread_id), "%lu:%s", thread_id, buffer);

        if (write(pipe_fd, buffer, strlen(buffer)) == -1) {
            perror("write to pipe");
            close(client_socket);
            close(pipe_fd);
            pthread_exit(NULL);
        }

        printf("Message from client %s sent to file_com pipe with thread ID: %lu\n", client_id, thread_id);

        // Read response from the pipe
        memset(buffer, 0, sizeof(buffer));
        if (read(pipe_fd, buffer, sizeof(buffer)) == -1) {
            perror("read from pipe");
            close(client_socket);
            close(pipe_fd);
            pthread_exit(NULL);
        }

        // Send the response back to the client
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("send to client");
            close(client_socket);
            close(pipe_fd);
            pthread_exit(NULL);
        }

        printf("Response from pipe sent to client %s: %s\n", client_id, buffer);
    }

    close(pipe_fd); // Close the pipe
    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create the named pipe
    if (mkfifo(pipe_name, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

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

        // Create a new thread to handle the client
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)&new_socket) < 0) {
            perror("could not create thread");
            close(new_socket);
            exit(EXIT_FAILURE);
        }

        // Detach the thread
        pthread_detach(client_thread);
    }

    return 0;
}
