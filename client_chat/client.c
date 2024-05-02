/**
 * @file client.c
 * @brief Programme client pour la partie client chat
 * 
 * Ce programme permet à un utilisateur de se connecter à un serveur de chat,
 * d'envoyer et de recevoir des messages, de créer et de supprimer des comptes,
 * et de gérer sa connexion via un terminal (menu).
 * 
 * @date 30 avril 2024
 * @author 
 * - Matéo Garcia
 * - Achille Gravouil
 */


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>

#define PORT 8080
#define PIPE_NAME_PREFIX "message_pipe_"
#define PIPE_NAME_SIZE 100
#define MAX_ID_LENGTH 50
#define IP_AD "127.0.0.1"

// Mutex pour l'accès au pipe
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Nom du pipe nommé
char pipe_name[PIPE_NAME_SIZE];

// sock tcp pour fermer dans le cleanup
int sock = 0;


/**
 * @brief Struct to hold socket, client ID, and connection status
 * 
 */
struct ThreadArgs {
    int sock;
    char* client_id;
    int pipe_fd;
    int *is_connected;
    char *pipe_name;
};


/**
 * @brief Fonction exécutée par le thread d'envoi de message
 * 
 * @param arg Arguments passés au thread (structure ThreadArgs)
 * @return void* Pointeur vers NULL
 */
void *send_message(void *arg) {
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    int sock = args->sock;
    int pipe_fd = args->pipe_fd;
    int *is_connected = args->is_connected;

    char message[2048];

    while (*is_connected) {
        // Read message from user 
        printf("\nEnter a message: ");
        fgets(message, sizeof(message), stdin);

        while (strlen(message) <= 1) {
            fgets(message, sizeof(message), stdin);
        }

        // Check for client disconnection
        if (strcmp(message, "exit\n") == 0) {
            break;
        }
        
        char pipe_message[2048 + 5];
        sprintf(pipe_message, "Me: %s", message);

        char prefixed_message[2048 + 5];
        // // Add Prefix S_MSG to the message 
        sprintf(prefixed_message, "S_MSG%s", message);

        // Send message to the server via socket
        send(sock, prefixed_message, strlen(prefixed_message), 0);

        // Send message to the pipe
        
        // Lock the mutex before writing to the pipe
        pthread_mutex_lock(&mutex);
        write(pipe_fd, pipe_message, strlen(pipe_message));
        pthread_mutex_unlock(&mutex);


    }

    return NULL;
}


/**
 * @brief Thread receiving messages from the server and send them to the pipe to afficheur_msg
 * 
 * @param arg Structure ThreadArgs containing the sock and pipe_fd 
 * @return void* 
 */
void *receive_and_send_to_pipe(void *arg) {
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    int sock = args->sock;
    int pipe_fd = args->pipe_fd;

    char buffer[2053];

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // Read message from the server
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread <= 0) {
            break;
        }

        // Lock the mutex before writing to the pipe
        pthread_mutex_lock(&mutex);
        write(pipe_fd, buffer, strlen(buffer));
        pthread_mutex_unlock(&mutex);
    }

    // Close the pipe
    close(pipe_fd);
    // Supprimer le pipe nommé
    if (unlink(pipe_name) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }

    return NULL;
}

// -----------------------------------------------------------     MENU  FUNCTION    ------------------------------------------------------------------------------------------

void clear_console() {
    system("clear");
}


/**
 * @brief Show choices in menu
 * 
 * @param is_connected to choose wich menu to display
 * 
 */
void display_menu(int is_connected) {
    clear_console();
    if (is_connected) {
        printf("Connected menu:\n");
        printf("1. Write a message\n");
        printf("2. Show list of all users\n");
        printf("3. Log out of the system\n");
        printf("4. Quit the client\n");
    } else {
        printf("Main menu:\n");
        printf("1. Connect to the system\n");
        printf("2. Create a new account\n");
        printf("3. Delete an existing account\n");
        printf("4. Quit the client\n");
    }
    printf("Enter your choice: ");
}


/**
 * @brief Log in to your account
 * Ask for pseudo and password 
 * Send msg with prefix A_ACC
 * 
 * @param sock 
 * @return char* 
 */
char * log_in(int sock) {
    char std_inPseudo[MAX_ID_LENGTH];
    char password[MAX_ID_LENGTH];

    char * pseudo= {0};

    do {
        printf("Enter your pseudo: \n");
        if (fgets(std_inPseudo, MAX_ID_LENGTH, stdin) == NULL) {
            perror("Error reading pseudo");
            return NULL;
        }

        // Supprime les caractères de nouvelle ligne à la fin
        std_inPseudo[strcspn(std_inPseudo, "\n")] = '\0';

        pseudo = std_inPseudo;

        // Supprime les espaces au début 
        while (*pseudo && *pseudo == ' ') {
            pseudo++;
        }

        // Supprime les espaces à la fin 
        char *end = pseudo + strlen(pseudo) - 1;
        while (end > pseudo && *end == ' ') {
            *end-- = '\0';
        }

        // Vérifier si le pseudo est vide après la suppression des espaces
        if (*pseudo == '\0') {
            printf("Pseudo cannot be empty.\n");
        }

    } while (*pseudo == '\0');


    printf("Enter your password: \n");
    if (fgets(password, MAX_ID_LENGTH, stdin) == NULL) {
        perror("Error reading password");
        return NULL;
    }
    password[strcspn(password, "\n")] = '\0'; 


    // Send pseudo with prefix A_ACC to the server
    char pseudo_serv[MAX_ID_LENGTH *2 + 6];
    sprintf(pseudo_serv, "A_ACC%s#%s", pseudo, password);

    if (send(sock, pseudo_serv, strlen(pseudo_serv), 0) == -1) {
        perror("Error sending pseudo to server");
        return NULL;
    }
    printf("sent to server !  \n");


    // Wait for authentication response from the server
    char auth[2053];
    if (recv(sock, &auth, sizeof(auth), 0) <= 0) {
        perror("Error receiving authentication response from server");
        return NULL;
    }

    printf("Response laaaaaaaaaaaaaaaaaaaaaaa    %s\n", auth);
    sleep(1);

    if (strcmp(auth, "Success") == 0) {
        // Return the pseudo as a dynamically allocated string
        printf("Pseudo: %s\n", pseudo);
        printf("Password: %s\n", password);
        // printf("Authentication response: %c\n", auth);
        return strdup(pseudo);
    } else {
        printf("Authentification failed... Try again\n");
        getchar();
        return NULL; 
    }

}


/**
 * @brief Print list of connected user
 * Send msg with prefix L_USE
 * 
 * @param sock 
 */
void show_user_list(int sock) {
    printf("Showing list of all users...\n");

    // Send message to server with prefix L_USE
    char msg[] = "L_USE";

    if (send(sock, msg, strlen(msg), 0) == -1) {
        perror("Error asking list of user to server");
        return;
    }


    pthread_mutex_lock(&mutex);

    // Receive the list from the server
    char list[2048]; 
    memset(list, 0, sizeof(list)); 
    if (recv(sock, list, sizeof(list), 0) <= 0) {
        perror("Problem with the user list, can't receive it from the server");
        return;
    }
    pthread_mutex_unlock(&mutex);

    // Print the list of connected users
    printf("List of connected users:\n");
    char *token = strtok(list, "\n"); 
    while (token != NULL) {
        printf("%s\n", token);
        token = strtok(NULL, "\n");
    }
   
    // Wait for user input before returning
    printf("Press Enter to continue...");
    getchar(); // Wait for user to press Enter
}


/**
 * @brief Create a new account
 * Send msg with prefix C_ACC
 * 
 * @param sock 
 */
void create_new_account(int sock) {
    printf("Creating a new account...\n");

    // Ask user for pseudo
    char pseudo[50];
    printf("Enter your pseudo: ");
    fgets(pseudo, sizeof(pseudo), stdin);

    // Remove trailing newline character from pseudo
    pseudo[strcspn(pseudo, "\n")] = '\0';

    // Ask user for password
    char password[50]; // Adjust size as needed
    do {
        printf("Enter a very strong password of at least 4 characters: ");
        fgets(password, sizeof(password), stdin);
        // Remove trailing newline character from password
        password[strcspn(password, "\n")] = '\0';
    } while (strlen(password) < 4);

    // Construct message with prefix C_ACC, pseudo, and password separated by a delimiter
    char msg[150]; // Adjust size as needed
    snprintf(msg, sizeof(msg), "C_ACC %s#%s", pseudo, password);

    // Send message to server
    if (send(sock, msg, strlen(msg), 0) == -1) {
        perror("Error sending create account request to server");
        return;
    }

    // Receive response from server
    char response[2053];
    if (recv(sock, &response, sizeof(response), 0) <= 0) {
        perror("Problem receiving response from server");
        return;
    }

    // Check if pseudo is already in use
    if (strcmp(response, "Failed") == 0) {
        printf("Error creating the compte. \nPseudo '%s' is already in use. Please choose another one.\n", pseudo);
        
        // Loop until a valid pseudo is entered
        bool valid_pseudo = false;

        while (!valid_pseudo) {
            printf("Enter a different pseudo: \n");
            fgets(pseudo, sizeof(pseudo), stdin);

            // Remove trailing newline character from pseudo
            pseudo[strcspn(pseudo, "\n")] = '\0';

            // Ask user for password
            char password[50]; // Adjust size as needed
            do {
                printf("Enter a very strong password of at least 4 characters: \n");
                fgets(password, sizeof(password), stdin);
                // Remove trailing newline character from password
                password[strcspn(password, "\n")] = '\0';
            } while (strlen(password) < 4);

            // Construct message with pseudo, and password separated by a delimiter
            char msg[2053]; // Adjust size as needed
            snprintf(msg, sizeof(msg), "C_ACC%s#%s", pseudo, password);
            
            // Send message to server
            if (send(sock, msg, strlen(msg), 0) == -1) {
                perror("Error sending pseudo to server");
                return;
            }
            // Receive response from server
            if (recv(sock, &response, sizeof(response), 0) <= 0) {
                perror("Problem receiving response from server\n");
                return;
            }
            if (strcmp(response, "Success") == 0) {
                printf("New account '%s' created.\n", pseudo);
                valid_pseudo = true;
            } else {
                printf("Pseudo '%s' is already in use. Please choose another one.\n", pseudo);

                char confirmation[100];
                printf("Do you want to try again ? ");
                fgets(confirmation, sizeof(confirmation), stdin);

                // Check for client disconnection
                if (strcmp(confirmation, "exit\n") == 0) {
                    printf("Going back to menu...\n");
                    break;
                }
            }
        }
    }else {
        printf("New account '%s' created.\n", pseudo);
        printf("Pseudo : %s\nPassword : %s\n", pseudo, password);
    }


    // Wait for user input before returning
    printf("Press Enter to continue...");
    getchar(); // Wait for user to press Enter
}


/**
 * @brief Delete existing account
 * Connect to the serveur and send msg with D_ACC
 * 
 */
void delete_existing_account(int sock) {
    printf("Deleting an existing account...\n");



    // Ask user for pseudo
    char pseudo[50];
    printf("Enter your pseudo: ");
    scanf(" %[^\n]", pseudo);

    // Ask user for password
    char password[50];
    do {
        printf("Enter your password :");
        scanf(" %[^\n]", password);
    } while (strlen(password) < 4);

        // Receive confirmation from user
    char conf; // Only need a single character
    printf("Are you sure? (y/n): ");
    scanf(" %c", &conf); // Note the space before %c to consume any leading whitespace

    if (conf != 'y' && conf != 'Y') {
        printf("Account deletion cancelled.\n");
        getchar();
        return;
    }
    
    // Clear input buffer
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}

    // Construct message with prefix D_ACC, pseudo, and password separated by #
    char msg[2053];
    snprintf(msg, sizeof(msg), "D_ACC%s#%s", pseudo, password);

    if (send(sock, msg, strlen(msg), 0) == -1) {
        perror("Error sending delete account request to server");
        return;
    }

    // Receive response from server
    char response[2053];
    if (recv(sock, &response, sizeof(response), 0) <= 0) {
        perror("Problem receiving response from server");
        return;
    }

    printf("Response : %s\n", response);
    
    // Account deleted
    if (strcmp(response, "Success") == 0) { 
        printf("Account deleted successfully.\n");
    } else { 
        // Account not deleted
        printf("Account deletion failed. You can try again later.\n");
    }

    // Wait for client input before returning
    printf("Press Enter to continue...");
    getchar(); 
}


/**
 * @brief log out of the account, unlink the pipe of afficheur_msg
 * Send msg with prefix L_ACC
 * 
 * @param sock to connect with server TCP
 * @param pseudo current pseudo of the connected account
 */
void log_out(int sock , char *pseudo){

    // // Password just to have something for the strtok
    // char password[50] = "mdpp"; 

    // // Construct message with prefix D_ACC, pseudo, and password separated by a delimiter
    // char msg[150];
    // snprintf(msg, sizeof(msg), "L_ACC%s#%s", pseudo, password);

    // if (send(sock, msg, strlen(msg), 0) == -1) {
    //     perror("Error sending delete account request to server");
    //     return;
    // }

}


/**
 * @brief Get the choice for the menu
 * 
 * @return int choice 
 */
int get_choice() {
    int choice;
    char input[100];

    if (fgets(input, sizeof(input), stdin) == NULL) {
        // Error reading input
        return -1; 
    }

    if (sscanf(input, "%d", &choice) != 1) {
        printf("Invalid choice, please enter a number.\n");
        return -1;
    }

    return choice;
}


/**
 * @brief Fonction de nettoyage à exécuter avant la sortie du programme
 */
void cleanup() {
    close(sock);
    // Supprimer le pipe nommé
    if (unlink(pipe_name) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    } else {
        printf("Pipe unlinked successfully.\n");
    }
    exit(EXIT_FAILURE);
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------


int main(int argc, char const *argv[]) {
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char* client_id = NULL; // Initialize client_id to NULL

    int is_connected = 0;
    int choice;


    // Create a struct to hold socket, client ID, and pipe file descriptor
    struct ThreadArgs args;


    if (signal(SIGINT, cleanup) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }


    while (1) {
        display_menu(is_connected);
        choice = get_choice();
        
        if (!is_connected) {

            switch (choice) {

                // Se connecter ----------------------------------------------------------------------------
                case 1:

                    // Create client socket
                    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        printf("\nSocket creation error\n");
                        return -1;
                    }

                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = htons(PORT);

                    // Convert IP address from text to binary form
                    if (inet_pton(AF_INET, IP_AD, &serv_addr.sin_addr) <= 0) {
                        printf("\nInvalid address/ Address not supported\n");
                        return -1;
                    }

                    // Connect to the server
                    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                        printf("\nConnection Failed\n");
                        return -1;
                    }


                    // Enter pseudo and mdp
                    client_id = log_in(sock);

                    if (client_id == NULL) {
                        close(sock);
                        break;
                    }
                    // Construct the pipe name with client ID
                    sprintf(pipe_name, "%s%s", PIPE_NAME_PREFIX, client_id);

                    args.pipe_name = pipe_name ;

                    // Create the named pipe
                    if (mkfifo(pipe_name, 0666) == -1 && errno != EEXIST) {
                        perror("Error creating named pipe");
                        return -1;
                    }


                    // Then we fork the afficheur_msg
                    pid_t pid = fork();

                    if (pid == -1) {
                        perror("fork failed");
                        exit(EXIT_FAILURE);
                    } else if (pid == 0) { // Child process
                        // Launch afficheur_msg in a new terminal
                        execlp("gnome-terminal", "gnome-terminal", "--", "../client_chat/afficheur_msg", pipe_name, NULL);
                        // perror("exec failed");
                        // exit(EXIT_FAILURE);
                    } else { // Parent process

                        // Open the pipe for writing
                        int pipe_fd = open(pipe_name, O_WRONLY);
                        if (pipe_fd == -1) {
                            perror("Failed to open pipe");
                            return -1;
                        }

                        args.sock = sock;
                        args.client_id = client_id;
                        args.pipe_fd = pipe_fd;
                        args.is_connected = &is_connected;


                        // Create a thread to receive messages from the server and send them to the pipe
                        pthread_t receive_thread;
                        if (pthread_create(&receive_thread, NULL, receive_and_send_to_pipe, (void *)&args) != 0) {
                            printf("Failed to create receive thread\n");
                            return -1;
                        }
                    
                    }


                    // Handle connection
                    is_connected = 1; // Mark client as connected

                    break;

                // Create Account ---------------------------------------------------------------------------
                case 2:

                    // Create client socket
                    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        printf("\nSocket creation error\n");
                        return -1;
                    }

                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = htons(PORT);

                    // Convert IP address from text to binary form
                    if (inet_pton(AF_INET, IP_AD, &serv_addr.sin_addr) <= 0) {
                        printf("\nInvalid address/ Address not supported\n");
                        return -1;
                    }

                    // Connect to the server
                    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                        printf("\nConnection Failed\n");
                        return -1;
                    }

                    create_new_account(sock);

                    close(sock);
                    break;

                // Delete account -------------------------------------------------------------------------
                case 3:

                    // Create client socket
                    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        printf("\nSocket creation error\n");
                        return -1;
                    }

                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = htons(PORT);

                    // Convert IP address from text to binary form
                    if (inet_pton(AF_INET, IP_AD, &serv_addr.sin_addr) <= 0) {
                        printf("\nInvalid address/ Address not supported\n");
                        return -1;
                    }

                    // Connect to the server
                    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                        printf("\nConnection Failed\n");
                        return -1;
                    }


                    delete_existing_account(sock);

                    close(sock);

                    
                    break;
                // ---------------------------------------------------------------------------------------

                
                // Quit -------------------------------------------------------------------------------------
                case 4:
                    // Quit the client
                    printf("Quitting the client...\n");
                    cleanup();
                    sleep(1);
                    exit(EXIT_SUCCESS);
                    break;
                

                // Default ----------------------------------------------------------------------------------
                default:
                    printf("Invalid choice\n");
                    break;
            }
        }
         
        else {
            switch (choice) {

                // Write a message : ---------------------------------------------------------------------
                case 1:{

                    args.sock = sock;
                    // Open the pipe for writing
                    int pipe_fd = open(args.pipe_name, O_WRONLY);
                    if (pipe_fd == -1) {
                        perror("Failed to open pipe");
                        return -1;
                    }
                    args.pipe_fd = pipe_fd;
                    args.is_connected = &is_connected;


                    // Create a thread to send messages to the server
                    pthread_t send_thread;
                    if (pthread_create(&send_thread, NULL, send_message, (void *)&args) != 0) {
                        printf("Failed to create send thread\n");
                        return -1;
                    }

                    // Wait for threads to finish
                    pthread_join(send_thread, NULL);

                    break;
                    
                }


                // List of connected user ----------------------------------------------------------------
                case 2:
                    show_user_list(sock);
                    break;


                // Log out of the system -----------------------------------------------------------------
                case 3:

                    log_out(sock, args.client_id) ;

                    // Close the socket
                    close(sock);

                    close(args.pipe_fd);
                    
                    is_connected = 0;

                    break;

                // Quit client ---------------------------------------------------------------------------
                case 4:
                    // Quit the client
                    printf("Quitting the client...\n");

                    log_out(sock, args.client_id) ;

                    // Close the sockets
                    close(sock);
                    close(args.pipe_fd);

                    // Unlink the pipe file
                    if (unlink(args.pipe_name) == -1) {
                        perror("Error unlinking pipe");
                        return -1;
                    }

                    exit(EXIT_SUCCESS);
                    break;

                // Default
                default:
                    printf("Invalid choice\n");
                    break;
            }
        }
    }        
    

    return 0;

}
