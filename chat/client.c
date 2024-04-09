#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define PORT 8080
#define PIPE_NAME "message_pipe"

// Mutex for pipe access
pthread_mutex_t pipe_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to send messages to the server
void *send_message(void *arg) {
    int sock = *((int *)arg);
    char message[1024];

    while (1) {
        // Read message from user input
        printf("\nEnter a message: ");
        fgets(message, sizeof(message), stdin);

        // Send message to the server via socket
        send(sock, message, strlen(message), 0);

        // Check for client disconnection
        if (strcmp(message, "exit\n") == 0) {
            break;
        }
    }

    return NULL;
}

// Function to receive messages from the server and send them to the pipe
void *receive_and_send_to_pipe(void *arg) {
    int sock = *((int *)arg);
    char buffer[1024];
    int pipe_fd;

    // Open the pipe for writing
    pipe_fd = open(PIPE_NAME, O_WRONLY);
    if (pipe_fd == -1) {
        // If the pipe already exists, open it in write mode only
        if (errno == EEXIST) {
            pipe_fd = open(PIPE_NAME, O_WRONLY);
        } else {
            perror("Failed to open pipe");
            exit(EXIT_FAILURE);
        }
    }

    while (1) {
        memset(buffer, 0, 1024);

        // Read message from the server
        int valread = read(sock, buffer, 1024);
        if (valread <= 0) {
            printf("\nServer disconnected\n");
            break;
        }
        printf("%s\n", buffer);

        // Lock the mutex before writing to the pipe
        pthread_mutex_lock(&pipe_mutex);

        // Send received message to the pipe
        write(pipe_fd, buffer, strlen(buffer));

        // Unlock the mutex
        pthread_mutex_unlock(&pipe_mutex);
    }

    // Close the pipe
    close(pipe_fd);

    return NULL;
}

int main(int argc, char const *argv[]) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    int client_id; // ID du client

    // Demande d'ID au lancement du client
    printf("Enter your client ID: ");
    scanf("%d", &client_id);

    // Créer le pipe nommé pour la communication avec l'afficheur de messages
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        // If the pipe already exists, do nothing
        if (errno != EEXIST) {
            perror("Error creating named pipe");
            return EXIT_FAILURE;
        }
    }

    // Fork process to launch message display
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        execlp("gnome-terminal", "gnome-terminal", "--", "./afficheur_msg", ">/dev/null", "2>&1", NULL);
        perror("exec failed");
        exit(EXIT_FAILURE);
    } else { // Parent process
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

        // Envoi de l'ID au serveur
        send(sock, &client_id, sizeof(int), 0);

        // Create a thread to send messages to the server
        pthread_t send_thread;
        if (pthread_create(&send_thread, NULL, send_message, (void *)&sock) != 0) {
            printf("Failed to create send thread\n");
            return -1;
        }

        // Create a thread to receive messages from the server and send them to the pipe
        pthread_t receive_thread;
        if (pthread_create(&receive_thread, NULL, receive_and_send_to_pipe, (void *)&sock) != 0) {
            printf("Failed to create receive thread\n");
            return -1;
        }

        // Wait for both threads to finish
        pthread_join(send_thread, NULL);
        pthread_join(receive_thread, NULL);

        // Close the socket
        close(sock);
    }

    return 0;
}
