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

                // AND Deconnect him from the list of connected user

                // Remove client from the array
                for (int i = index; i < num_clients - 1; i++) {
                    strcpy(clients[i].id, clients[i + 1].id);
                    clients[i].socket = clients[i + 1].socket;
                }
                num_clients--;
            }
            pthread_exit(NULL);
        }


        // // Écrire le message dans le pipe avec l'ID du thread
        // char message_with_thread_id[2053]; // Augmenter la taille si nécessaire
        // snprintf(message_with_thread_id, sizeof(message_with_thread_id), "%lu:%s", thread_id, buffer);



        printf("Message from client %s: %s\n", client_id, buffer);

        char type_msg[6]; // Increase size by 1 for null terminator
        strncpy(type_msg, buffer, 5);
        type_msg[5] = '\0'; // Null-terminate the string
        printf("%s\n", type_msg);

        char message_content[valread - 5]; // Subtract 5 for the first 5 characters
        strncpy(message_content, buffer + 5, valread - 5);
        message_content[valread - 5] = '\0'; // Null-terminate the string

        printf("Msg_content : %s\n", message_content);


        if (strcmp(type_msg, "A_ACC") == 0) { // -------------------------------------------------  Authentication   --------------------------------------------------------
            printf("Authentication\n");

            sprintf(client_id, "%s", message_content);

 
            // Authentication here
            if (write(pipe_fd, buffer, strlen(buffer)) == -1) {
                perror("write to pipe");
                close(client_socket);
                pthread_exit(NULL);
            }


            

            // For now it's always good
            char valid[2053] ; // = 'T';

            // Read response from the pipe
            // memset(buffer, 0, sizeof(buffer));
            if (read(pipe_fd, valid, sizeof(valid)) == -1) {
                perror("read from pipe");
                close(client_socket);
                close(pipe_fd);
                pthread_exit(NULL);
            }


            if (send(client_socket, &valid, sizeof(valid), 0) == -1) {
                perror("Error sending authentication response to client");
                close(client_socket);
                pthread_exit(NULL);
            }


            // Store the client information
            if (num_clients < MAX_CLIENTS) {
                strcpy(clients[num_clients].id, client_id);
                clients[num_clients].socket = client_socket;
                num_clients++;
                printf("Client %s connected\n", client_id);
            } else {
                printf("Max clients reached. Rejecting connection from client %s.\n", client_id);
                close(client_socket);
                pthread_exit(NULL);
            }   



        } else { 
            // Other message types
            switch (type_msg[0]) {
                case 'S':  //-----------------------------------------------------  Send Message  -----------------------------------------
                    if (strcmp(type_msg, "S_MSG") == 0) {
                        // Send the message to all clients except the one who sent it

                        char message_content[valread - 5]; // Subtract 5 for the first 5 characters
                        strncpy(message_content, buffer + 5, valread - 5);
                        message_content[valread - 5] = '\0'; // Null-terminate the string

                        for (int i = 0; i < num_clients; i++) {
                            if (clients[i].socket != client_socket) {
                                // Construct the message with client ID prefix
                                char prefixed_message[2053];
                                snprintf(prefixed_message, sizeof(prefixed_message), "%s: %s", client_id, message_content);
                                send(clients[i].socket, prefixed_message, strlen(prefixed_message), 0);
                            }
                        }
                    } else {
                        printf("Unknown message type: %s\n", type_msg);
                    }
                    break;

                // Get list of connected user
                case 'L':
                    if (strcmp(type_msg, "L_USE") == 0) { //-----------------------  List of User  -----------------------------------------
                        // Get list of connected users
                        
                        // Prepare a buffer to store the list of connected users
                        char list[2048] = {0};
                        
                        // Iterate through the array of connected clients
                        for (int i = 0; i < num_clients; i++) {
                            // Append the client's ID (pseudo) to the list buffer
                            strcat(list, clients[i].id);
                            strcat(list, "\n"); // Add newline for better formatting
                        }

                        // Send the list to the client
                        if (send(client_socket, list, strlen(list), 0) == -1) {
                            perror("Error sending list of users to client");
                            close(client_socket);
                            pthread_exit(NULL);
                        }


                    } else {
                        printf("Unknown message type: %s\n", type_msg);
                    }
                    break;
                
                // Create Account 
                case 'C':
                    if (strcmp(type_msg, "C_ACC") == 0) { //------------------------  Create Account  ---------------------------------------
                        // Receive pseudo and password from client
                        char pseudo[50]; // Adjust size as needed
                        char password[50]; // Adjust size as needed

                        // Extract pseudo and password from the message_content
                        sscanf(message_content, "%49[^#]#%49s", pseudo, password);

                        printf("Pseudo : %s\nPassword : %s\n", pseudo, password);

                        // Check if possible
                        // Transfer the message to the pipe

                        // we receive the response 
                        char *response = "Success" ; // Implement here we are supposed to get the response from the pipe


                        // Send response to client 
                        
                        if (send(client_socket, &response, sizeof(response), 0) == -1) {
                            perror("Error sending response to client");
                            break;
                        }



                        
                    } else {
                        printf("Unknown message type: %s\n", type_msg);
                    }
                    break;

                // Delete Account
                case 'D':
                    if (strcmp(type_msg, "D_ACC") == 0) { //-------------------------  Delete Account  ---------------------------------------
                        // Delete Account

                        // Receive pseudo and password from client
                        char pseudo[50]; // Adjust size as needed
                        char password[50]; // Adjust size as needed

                        // Extract pseudo and password from the message_content
                        sscanf(message_content, "%49[^#]#%49s", pseudo, password);

                        printf("Pseudo : %s\nPassword : %s\n", pseudo, password);

                        
                        // Delete account

                        // Implement here to send to the pipe

                        // Dummy response (We should receive the response from the pipe)
                        char response = 'Y';


                        if (send(client_socket, &response, sizeof(response), 0) == -1) {
                            perror("Error sending account deletion success response to client");
                            break;
                        }
   

                    } else {
                        printf("Unknown message type: %s\n", type_msg);
                    }
                    break;

                // Default
                default:
                    printf("Unknown message type: %s\n", type_msg);
                    break;
            }
        }
    }

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
