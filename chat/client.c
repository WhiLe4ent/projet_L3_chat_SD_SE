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
#define MAX_ID_LENGTH 50

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



// -----------------------------------------------------------     MENU  FUNCTION    ------------------------------------------------------------------------------------------


void clear_console() {
    printf("\033[2J\033[H"); // Clear the console (Linux)
}

void display_menu(int is_connected) {
    clear_console();
    if (is_connected) {
        printf("Connected menu:\n");
        printf("1. Write a message\n");
        printf("2. Log out of the system\n");
        printf("3. Show list of all users\n");
        printf("4. Create a new account\n");
        printf("5. Delete an existing account\n");
        printf("6. Quit the client\n");
    } else {
        printf("Main menu:\n");
        printf("1. Connect to the system\n");
        printf("2. Show list of all users\n");
        printf("3. Create a new account\n");
        printf("4. Delete an existing account\n");
        printf("5. Quit the client\n");
    }
    printf("Enter your choice: ");
}

void handle_connected_menu(int sock) {
    printf("Writing message...\n");
    fflush(stdout); // Flush stdout to ensure message is displayed
    send_message(&sock); // Pass the socket to send_message function
}

void handle_main_menu(int sock) {
    printf("Main menu choice...\n");
    clear_console(); // Clear the console after timeout
}

void handle_connection(int sock) {
    char pseudo[MAX_ID_LENGTH];
    char password[MAX_ID_LENGTH];

    printf("Enter your pseudo: \n");
    fgets(pseudo, MAX_ID_LENGTH, stdin);
    pseudo[strcspn(pseudo, "\n")] = '\0'; // Remove newline character

    printf("Enter your password: \n");
    fgets(password, MAX_ID_LENGTH, stdin);
    password[strcspn(password, "\n")] = '\0'; // Remove newline character

    // Implement authentication logic here
    // For now, just print the pseudo and password
    printf("Pseudo: %s\n", pseudo);
    printf("Password: %s\n", password);

    // Send the ID to the server
    send(sock, pseudo, strlen(pseudo), 0);
}

void show_user_list() {
    printf("Showing list of all users...\n");
}

void create_new_account() {
    printf("Creating a new account...\n");
}

void delete_existing_account() {
    printf("Deleting an existing account...\n");
}

int get_choice() {
    int choice;
    char input[100];

    if (fgets(input, sizeof(input), stdin) == NULL) {
        return -1; // Error reading input
    }

    if (sscanf(input, "%d", &choice) != 1) {
        printf("Invalid choice, please enter a number.\n");
        return -1;
    }

    return choice;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------







int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    int client_id; // ID du client

    int is_connected = 0;
    int choice;

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

    while (1) {
        display_menu(is_connected);
        choice = get_choice();
        
        if (is_connected) {
            switch (choice) {
                case 1:{
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
                        // Handle connected menu
                        handle_connected_menu(sock);
                        break;
                    }
                }
                case 2:
                    // Log out of the system
                    is_connected = 0;
                    break;
                case 3:
                    show_user_list();
                    break;
                case 4:
                    create_new_account();
                    break;
                case 5:
                    delete_existing_account();
                    break;
                case 6:
                    // Quit the client
                    printf("Quitting the client...\n");
                    exit(EXIT_SUCCESS);
                    break;
                default:
                    printf("Invalid choice\n");
                    break;
            }
        } else {
            switch (choice) {
                case 1:

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


                    // Handle connection
                    handle_connection(sock);
                    is_connected = 1; // Mark client as connected

                    break;
                case 2:
                    show_user_list();
                    break;
                case 3:
                    create_new_account();
                    break;
                case 4:
                    delete_existing_account();
                    break;
                case 5:
                    // Quit the client
                    printf("Quitting the client...\n");
                    exit(EXIT_SUCCESS);
                    break;
                default:
                    printf("Invalid choice\n");
                    break;
            }
        }
    }

    return 0;

}
