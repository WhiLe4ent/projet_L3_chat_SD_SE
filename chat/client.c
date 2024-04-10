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
#define PIPE_NAME_PREFIX "message_pipe_"
#define PIPE_NAME_SIZE 20

// Mutex for pipe access
pthread_mutex_t pipe_mutex = PTHREAD_MUTEX_INITIALIZER;

// Struct to hold socket and client ID
struct ThreadArgs {
    int sock;
    int client_id;
    int pipe_fd;
};

// Function to send messages to the server
void *send_message(void *arg) {
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    int sock = args->sock;
    int client_id = args->client_id;
    int pipe_fd = args->pipe_fd;

    char message[1024];

    while (1) {
        // Read message from user input
        printf("\nEnter a message: ");
        fgets(message, sizeof(message), stdin);

        while (strlen(message) <= 1) {
            fgets(message, sizeof(message), stdin);
        }

        // Send message to the server via socket
        send(sock, message, strlen(message), 0);

        // Send message to the pipe
        char prefixed_message[1024 + 5];
        sprintf(prefixed_message, "Me: %s", message);
        
        // Lock the mutex before writing to the pipe
        pthread_mutex_lock(&pipe_mutex);
        write(pipe_fd, prefixed_message, strlen(prefixed_message));
        pthread_mutex_unlock(&pipe_mutex);

        // Check for client disconnection
        if (strcmp(message, "exit\n") == 0) {
            close(pipe_fd);
            break;
        }
    }

    return NULL;
}

// Function to receive messages from the server and send them to the pipe
void *receive_and_send_to_pipe(void *arg) {
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    int sock = args->sock;
    int client_id = args->client_id;
    int pipe_fd = args->pipe_fd;

    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // Read message from the server
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread <= 0) {
            printf("\nServer disconnected\n");
            break;
        }

        // Lock the mutex before writing to the pipe
        pthread_mutex_lock(&pipe_mutex);
        write(pipe_fd, buffer, strlen(buffer));
        pthread_mutex_unlock(&pipe_mutex);
    }

    // Close the pipe
    close(pipe_fd);

    return NULL;
}



int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    int client_id; // ID du client

    // Demande d'ID au lancement du client
    printf("Enter your client ID: ");
    scanf("%d", &client_id);

    // Construct the pipe name with client ID
    char pipe_name[PIPE_NAME_SIZE];
    sprintf(pipe_name, "%s%d", PIPE_NAME_PREFIX, client_id);

    // Create the named pipe
    if (mkfifo(pipe_name, 0666) == -1 && errno != EEXIST) {
        perror("Error creating named pipe");
        return -1;
    }

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

    printf("Coucou ça commence");

    fflush(stdout);


    // Fork process to launch message display in a new terminal
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        // Launch afficheur_msg in a new terminal
        execlp("gnome-terminal", "gnome-terminal", "--", "./afficheur_msg", pipe_name, NULL);
        perror("exec failed");
        exit(EXIT_FAILURE);
    } else { // Parent process
        // Create a struct to hold socket, client ID, and pipe file descriptor
        struct ThreadArgs args;
        args.sock = sock;
        args.client_id = client_id;
        // Open the pipe for writing
        int pipe_fd = open(pipe_name, O_WRONLY);
        if (pipe_fd == -1) {
            perror("Failed to open pipe");
            return -1;
        }
        args.pipe_fd = pipe_fd;


        // Create a thread to receive messages from the server and send them to the pipe
        pthread_t receive_thread;
        if (pthread_create(&receive_thread, NULL, receive_and_send_to_pipe, (void *)&args) != 0) {
            printf("Failed to create receive thread\n");
            return -1;
        }

        // Create a thread to send messages to the server
        pthread_t send_thread;
        if (pthread_create(&send_thread, NULL, send_message, (void *)&args) != 0) {
            printf("Failed to create send thread\n");
            return -1;
        }

        // Wait for both threads to finish
        pthread_join(send_thread, NULL);
        pthread_join(receive_thread, NULL);

        // Close the socket
        close(sock);

        // Unlink the pipe file
        if (unlink(pipe_name) == -1) {
            perror("Error unlinking pipe");
            return -1;
        }
    }

    return 0;
}
