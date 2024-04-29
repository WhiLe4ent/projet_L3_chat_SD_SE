#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h> // Pour les constantes de mode d'ouverture des pipes
#include <sys/stat.h> // Pour la fonction mkfifo
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define PIPE_COM_TO_FILE_MSG "./pipe_com_to_file_msg"
#define PIPE_TO_COM "./pipe_to_com"


#define PORT 8080
#define MAX_CLIENTS 10 // Maximum number of clients
#define MAX_ID_LENGTH 50 // Maximum length of client ID

#define MAX_USERS 10
#define MAX_PSEUDO_LENGTH 50


#define ROWS 10
#define COLS 50


int pipe_to_com_read, pipe_com_to_file_write;


char *shared_memory;


typedef struct {
    char pseudo[MAX_PSEUDO_LENGTH];
} User;


// Structure to hold client information
typedef struct {
    char id[MAX_ID_LENGTH];
    pthread_t thread_id;
    int socket;
} Client;


// Global array to store connected clients
Client clients[MAX_CLIENTS];
int num_clients = 0; // Number of connected clients


void cleanup() {
    // Code de nettoyage à exécuter avant la sortie

    // Supprimer les pipes

    if (unlink(PIPE_COM_TO_FILE_MSG) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }
    if (unlink(PIPE_TO_COM) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }

    printf("Pipes unlinked successfully.\n");
    exit(EXIT_FAILURE);
}


void create_shared_memory(char **shared_memory) {
    // Générer une clé unique avec ftok
    key_t key = ftok("./memoire_partagee/mem_part.txt", 65);

    // Créer ou localiser un segment de mémoire partagée
    int shmid = shmget(key, sizeof(char) * ROWS * COLS, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attacher le segment de mémoire partagée
    *shared_memory = (char *)shmat(shmid, NULL, 0);
    if (*shared_memory == (char *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
}


void detach_shared_memory(char *shared_memory) {
    // Détacher le segment de mémoire partagée
    if (shmdt(shared_memory) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
}


void get_connected_users(char *shared_memory, User *users) {
    // Parcourir la mémoire partagée et copier chaque pseudo dans la liste
    int count = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        char *current_pseudo = shared_memory + i * MAX_PSEUDO_LENGTH;
        if (strcmp(current_pseudo, "") != 0) {
            strcpy(users[count].pseudo, current_pseudo);
            count++;
        }
    }
}


void write_pipe_com( char *message, int pipe_com_to_file_write){


    // Authentication here
    if (write(pipe_com_to_file_write, message, strlen(message)) == -1) {
        perror("write to pipe");
        pthread_exit(NULL);
    }


}


void read_pipe_com(char *response, int pipe_to_com_read) {

    // Read from the named pipe
    ssize_t bytes_read = read(pipe_to_com_read, response, 2053 - 1);
    if (bytes_read == -1) {
        perror("read from pipe");
        pthread_exit(NULL);
    }

    // Null-terminate the response buffer
    response[bytes_read] = '\0';
}


void send_tcp( int client_socket, char *message){

    if (send(client_socket, message, strlen(message), 0) == -1) {
        perror("Error sending authentication response to client");
        close(client_socket);
        pthread_exit(NULL);
    }

}


void read_and_send_response( int client_socket, const char *tid_char, char *type_msg, int pipe_to_com_read) {
    char response[2053] = {0}; // Buffer for storing the response
    char tid_verif[2053] = {0}; // Buffer for storing the thread identifier received from the response

    do {
        // Read the response from the pipe
        read_pipe_com(response, pipe_to_com_read);
        printf("Initial response from pipe com : %s\n", response);

        // Extract the thread identifier and response from the received message
        strcpy(tid_verif, strtok(response, "#"));
        char *response = strtok(NULL, "");
        printf("Thread id response %s\n", tid_verif);

        // Check if the received thread identifier matches the current thread identifier
        if (strcmp(tid_verif, tid_char) == 0 && strcmp(type_msg, "L_ACC") != 0) {
            printf("Response : %s\n", response);

            // Send the response to the client
            send_tcp(client_socket, response);
        } else {
            // The thread identifiers do not match, continue reading until the correct thread is received
            printf("Received response from wrong thread. Waiting for correct thread...\n");
        }
    } while (strcmp(tid_verif, tid_char) != 0);
}



// Function to handle each client connection
void *handle_client(void *arg) {
    int client_socket = *((int*)arg);
    char client_id[MAX_ID_LENGTH] = {0};
    int index = -1;

    char buffer[2048 + 5 + 100] = {0};


    // Créer l'espace de mémoire partagée
    create_shared_memory(&shared_memory);


    // Open the named pipe for reading and writing
    pipe_to_com_read = open(PIPE_TO_COM, O_RDWR);
    if (pipe_to_com_read == -1) {
        perror("open");
        // close(client_socket);
        pthread_exit(NULL);
    }
    // Open the named pipe for reading and writing
    pipe_com_to_file_write = open(PIPE_COM_TO_FILE_MSG, O_RDWR);
    if (pipe_com_to_file_write == -1) {
        perror("open");
        // close(client_socket);
        pthread_exit(NULL);
    }


    pthread_t thread_id = pthread_self(); // Get the ID of the current thread


    while (1) {
        memset(buffer, 0, sizeof(buffer));

        printf("blablabla\n");
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
        printf("Message from client %s: %s\n", client_id, buffer);



        char type_msg[6]; // Increase size by 1 for null terminator
        strncpy(type_msg, buffer, 5);
        type_msg[5] = '\0'; // Null-terminate the string
        printf("%s\n", type_msg);

        char message_content[valread - 5]; // Subtract 5 for the first 5 characters
        strncpy(message_content, buffer + 5, valread - 5);
        message_content[valread - 5] = '\0'; // Null-terminate the string



        char tid_char[20] ;
        int thread_id_length = snprintf(tid_char, sizeof(tid_char), "%lu", thread_id);


        printf("Msg_content : %s\n", message_content);
        printf("TID : %lu\n", thread_id);
        printf("TID : %d\n", thread_id_length);
        printf("TID : %s\n", tid_char);


        size_t total_length = strlen(buffer) + strlen(tid_char) + 1; // +1 pour le caractère nul

        char message[2053 + 50 ] = {0};
        snprintf(message, total_length +1, "%s#%s%s", tid_char, type_msg, message_content); // +1 pour le #



        // Ajoutez le caractère nul à la fin de la chaîne buffer
        // buffer[total_length - 1] = '\0';
        printf("Message with tid : %s\n", message);
        printf("message_content : %s\n", message_content);

        if (strcmp(type_msg, "A_ACC") == 0) { // -------------------------------------------------  Authentication   --------------------------------------------------------
            printf("Authentication\n");


            write_pipe_com(message, pipe_com_to_file_write);

           
            read_and_send_response( client_socket, tid_char, type_msg, pipe_to_com_read);


            // Extraire le client_id de la première partie de message_content
            strcpy(client_id , strtok(message_content, "#"));
            printf("Client_id : %s\n", client_id);

            // Stocker les informations du client
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

            printf("That's done... Let's see\n");


        } else { 
            // Other message types
            switch (type_msg[0]) {
                case 'S':  //-----------------------------------------------------  Send Message  -----------------------------------------
                    if (strcmp(type_msg, "S_MSG") == 0) {
                        // Send the message to all clients except the one who sent it

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

                        // Récupérer la liste des utilisateurs connectés depuis la mémoire partagée
                        User users[MAX_USERS];
                        get_connected_users(shared_memory, users);

                        // Afficher la liste des utilisateurs connectés
                        printf("Liste des utilisateurs connectés :\n");
                        for (int i = 0; i < MAX_USERS; i++) {
                            if (strlen(users[i].pseudo) > 0) {
                                printf("- %s\n", users[i].pseudo);
                            }
                        }
                        

                        // Declare a buffer to store the list of connected users
                        char list[MAX_USERS * MAX_PSEUDO_LENGTH + MAX_USERS] = {0}; // + MAX_USERS for newline characters

                        // Iterate through the array of connected users
                        for (int i = 0; i < MAX_USERS; i++) {
                            // Append the user's pseudo to the list buffer
                            strcat(list, shared_memory + i * MAX_PSEUDO_LENGTH);
                            strcat(list, "\n"); // Add newline for better formatting
                        }

                        // Send the list to the client
                        send_tcp(client_socket, list);
                        


                    }
                    else if (strcmp(type_msg, "L_ACC") == 0)
                    {
                        // Receive pseudo and password from client
                        char pseudo[50] = {0}; // Adjust size as needed
                        char password[50] = {0}; // Adjust size as needed

                        // Extract pseudo and password from the message_content
                        sscanf(message_content, "%49[^#]#%49s", pseudo, password);
                        printf("Pseudo : %s\nPassword : %s\n", pseudo, password);

                        write_pipe_com(message, pipe_com_to_file_write);
                        read_and_send_response( client_socket, tid_char, type_msg, pipe_to_com_read);

                        // Détacher l'espace de mémoire partagée
                        detach_shared_memory(shared_memory);


                    }
                    else {
                        printf("Unknown message type: %s\n", type_msg);
                    }
                    break;
                
                // Create Account 
                case 'C':
                    if (strcmp(type_msg, "C_ACC") == 0) { //------------------------  Create Account  ---------------------------------------
                        // Receive pseudo and password from client
                        char pseudo[50] = {0}; // Adjust size as needed
                        char password[50] = {0}; // Adjust size as needed

                        // Extract pseudo and password from the message_content
                        sscanf(message_content, "%49[^#]#%49s", pseudo, password);
                        printf("Pseudo : %s\nPassword : %s\n", pseudo, password);

                        write_pipe_com(message , pipe_com_to_file_write);
                        read_and_send_response( client_socket, tid_char, type_msg, pipe_to_com_read);

                    } else {
                        printf("Unknown message type: %s\n", type_msg);
                    }
                    break;

                // Delete Account
                case 'D':
                    if (strcmp(type_msg, "D_ACC") == 0) { //-------------------------  Delete Account  ---------------------------------------
                        // Delete Account

                        // Receive pseudo and password from client
                        char pseudo[50] = {0}; // Adjust size as needed
                        char password[50] = {0}; // Adjust size as needed

                        // Extract pseudo and password from the message_content
                        sscanf(message_content, "%49[^#]#%49s", pseudo, password);

                        printf("Pseudo : %s\nPassword : %s\n", pseudo, password);

                        write_pipe_com(message, pipe_com_to_file_write);

                        read_and_send_response( client_socket, tid_char, type_msg, pipe_to_com_read) ;


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

    // Détacher l'espace de mémoire partagée
    detach_shared_memory(shared_memory);

    pthread_exit(NULL);
}



int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    

    // Create the named pipe ---------------------------------------------------------
    if (mkfifo(PIPE_COM_TO_FILE_MSG, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    if (mkfifo(PIPE_TO_COM, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }


    // Create server socket ---------------------------------------------------------
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options ---------------------------------------------------------
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket ---------------------------------------------------------
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections ---------------------------------------------------------
    if (listen(server_fd, 10) < 0) {
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
        printf("Welcome in the server\n");

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
