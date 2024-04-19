#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define PORT 8080
#define MAX_CLIENTS 10 // Maximum number of clients
#define MAX_ID_LENGTH 50 // Maximum length of client ID

// Structure to hold client information
typedef struct {
    char id[MAX_ID_LENGTH];
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

        printf("Message from client %s: %s\n", client_id, buffer);

        char type_msg[6]; // Increase size by 1 for null terminator
        strncpy(type_msg, buffer, 5);
        type_msg[5] = '\0'; // Null-terminate the string
        printf("%s\n", type_msg);

        char message_content[valread - 5]; // Subtract 5 for the first 5 characters
        strncpy(message_content, buffer + 5, valread - 5);
        message_content[valread - 5] = '\0'; // Null-terminate the string

        printf("Msg_content : %s\n", message_content);


        if (strcmp(type_msg, "A_ACC") == 0) { // Authentication
            printf("Authentication\n");

            sprintf(client_id, "%s", message_content);

            // Send authentication acknowledge back to the client
            // After receiving the pseudo, send an acknowledgment back to the client
            char ack = 'A';
            if (send(client_socket, &ack, sizeof(ack), 0) == -1) {
                perror("Error sending acknowledgment to client");
                close(client_socket);
                pthread_exit(NULL);
            }

            // Wait to receive client password1

            char password[50]; // Adjust 50 as needed
            if (recv(client_socket, password, 50, 0) <= 0) {
                perror("recv failed");
                close(client_socket);
                pthread_exit(NULL);
            }
            printf("Password: %s\n", password);

            
            // Authentication here

            

            // For now it's always good
            char valid = 'T';
            if (send(client_socket, &valid, sizeof(valid), 0) == -1) {
                perror("Error sending authentication response to client");
                close(client_socket);
                pthread_exit(NULL);
            }


            if (valid == 'T'){

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
            }else{
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

                        // Check if the pseudo is already in use
                        bool pseudo_in_use = false; // Implement your logic to check if the pseudo is in use

                        if (pseudo_in_use) {
                            // Send response to client indicating pseudo is already in use
                            char response = 'U';
                            if (send(client_socket, &response, sizeof(response), 0) == -1) {
                                perror("Error sending response to client");
                                break;
                            }

                            // Loop until a valid pseudo is received
                            while (pseudo_in_use) {
                                // Receive new pseudo and password from client
                                if (recv(client_socket, message_content, sizeof(message_content), 0) <= 0) {
                                    perror("Error receiving pseudo and password from client");
                                    break;
                                }

                                // Extract pseudo and password from the message_content
                                sscanf(message_content, "%49[^#]#%49s", pseudo, password);

                                // Check if the new pseudo is in use
                                // Update pseudo_in_use variable accordingly
                            }

                            // If pseudo is not in use, send response to client indicating acceptance
                            response = 'V';
                            if (send(client_socket, &response, sizeof(response), 0) == -1) {
                                perror("Error sending response to client");
                                break;
                            }
                        } else {
                            // Send response to client indicating acceptance of the pseudo
                            char response = 'V';
                            if (send(client_socket, &response, sizeof(response), 0) == -1) {
                                perror("Error sending response to client");
                                break;
                            }
                        }

                        // Create the account
                        // Implement your logic to create the account with the received pseudo and password

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

                        
                        char conf = 'y';
                        if (conf == 'y' || conf == 'Y') {
                            // Delete account

                            // Dummy response 
                            char response = 'Y';
                            if (send(client_socket, &response, sizeof(response), 0) == -1) {
                                perror("Error sending account deletion success response to client");
                                break;
                            }
                        } else {
                            
                            printf("Cancel deleting account...");
                            getchar();
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
