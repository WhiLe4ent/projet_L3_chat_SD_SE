#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#define PIPE_TO_GESTION "./pipe_to_gestion"
#define PIPE_TO_FILE_MSG "./pipe_gest_to_file_msg"
#define MAX_MESSAGE_SIZE 2053
#define PORT 4003

#define ROWS 10
#define COLS 50

// Global variable to track the next available index in shared memory
int next_index = 0; 

char *shared_memory = {0};

int shmid ;

/**
 * @brief Localised a shared memory 
 * 
 * @param shared_memory 
 */
void create_shared_memory() {
    // Get the key
    key_t key = ftok("./memoire_partagee/mem_part.txt", 65);

    // Locate the memory
    shmid = shmget(key, sizeof(char) * ROWS * COLS, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // attache shared memory
    shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
}


/**
 * @brief Detach shared memory
 * 
 * @param shared_memory 
 */
void detach_shared_memory() {
    // Détacher le segment de mémoire partagée
    if (shmdt(shared_memory) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    // // Supprimer la mémoire partagée
    // if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    //     perror("shmctl");
    //     exit(EXIT_FAILURE);
    // }
}


/**
 * @brief Add a pseudo in the shared memory 
 * 
 * @param pseudo Pseudo to add 
 */
void add_to_shared_memory(const char *pseudo) {
    // Vérifier si l'index dépasse la taille maximale
    if (next_index >= ROWS) {
        printf("Cannot add more pseudonyms. Shared memory is full.\n");
        return;
    }

    // Copier le pseudonyme dans la mémoire partagée à l'emplacement de l'index actuel
    strncpy(shared_memory + next_index * COLS, pseudo, COLS);
    printf("Pseudo '%s' added to shared memory at index %d.\n", pseudo, next_index);

    // Incrémenter l'index pour pointer vers le prochain emplacement disponible
    next_index++;
}


/**
 * @brief Remove a pseudo from shared memory
 * 
 * @param pseudo Pseudo to remove
 */
void remove_from_shared_memory(const char *pseudo) {

    // Parcourir la mémoire partagée pour trouver le pseudo à supprimer
    for (int i = 0; i < ROWS; i++) {
        char *current_pseudo = shared_memory + i * COLS;
        if (strcmp(current_pseudo, pseudo) == 0) {
            
            next_index--;
            // Trouvé le pseudo à supprimer, puis déplacer les pseudos suivants vers le haut
            for (int j = i; j < ROWS - 1; j++) {
                char *next_pseudo = shared_memory + (j + 1) * COLS;
                strcpy(current_pseudo, next_pseudo);
                current_pseudo = next_pseudo;
            }

            // Effacer le dernier pseudonyme
            memset(current_pseudo, 0, COLS);

            printf("Pseudo '%s' removed from shared memory.\n", pseudo);
            return;
        }
    }

    // Pseudo non trouvé
    printf("Pseudo '%s' not found in shared memory.\n", pseudo);
}


/**
 * @brief Look if the pseudo is in the shared memory 
 * If it's not in the shared memory it return true (1) and if it is false (0)
 * 
 * @param pseudo Pseudo to look for
 * @return int 1 (true) if it is not found and 0 (False) if it is
 */
int pseudo_not_in_shared_memory(const char *pseudo) {
   
    // Parcourir la mémoire partagée pour trouver le pseudo à supprimer
    for (int i = 0; i < ROWS; i++) {
        char *current_pseudo = shared_memory + i * COLS;
        if (strcmp(current_pseudo, pseudo) == 0) {
            
            printf("Pseudo '%s' found in shared memory.\n", pseudo);
            return 0; 
        }
    }
    // Pseudo not found
    return 1;
}


/**
 * @brief Send udp message to client RMI
 * 
 * @param message message to transmit
 * @param sockfd socket UDP
 * @param servaddr struct with sock address
 */
void send_udp_message(const char *message, int sockfd, struct sockaddr_in *servaddr) {
    // Sending message
    if (sendto(sockfd, (const char *)message, strlen(message), 0, (const struct sockaddr *)servaddr, sizeof(*servaddr)) < 0) {
        perror("Erreur lors de l'envoi du message");
        exit(EXIT_FAILURE);
    }

    printf("Message envoyé avec succès : %s\n", message);
}


/**
 * @brief Receive response from the client RMI
 * 
 * @param sockfd socket UDP
 * @param servaddr struct with sock address
 * @return char* the response
 */
char *receive_udp_response(int sockfd, struct sockaddr_in *servaddr) {
    struct sockaddr_in cliaddr;
    socklen_t len;

    // Utilisation d'une variable statique pour éviter la fuite de mémoire
    static char buffer[MAX_MESSAGE_SIZE]; 
    memset(&cliaddr, 0, sizeof(cliaddr));

    len = sizeof(cliaddr);

    // Réception du message
    ssize_t n = recvfrom(sockfd, buffer, MAX_MESSAGE_SIZE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
    if (n < 0) {
        perror("Erreur lors de la réception du message");
        exit(EXIT_FAILURE);
    }

    buffer[n] = '\0';
    printf("Réponse du serveur UDP : %s\n", buffer);

    return buffer;  
}


/**
 * @brief Send response to the pipe
 * 
 * @param pipefd the pipe to use
 * @param response Response to send
 */
void send_response_pipe(int pipefd, char *response) {
    // Envoyer la réponse du serveur UDP sur le pipe gestion
    ssize_t bytes_written = write(pipefd, response, strlen(response));
    if (bytes_written == -1) {
        perror("Erreur lors de l'écriture sur le pipe gestion");
        exit(EXIT_FAILURE);
    }
    printf("Done\n");
}

/**
 * @brief Fonction to clean the ressources to execute at the end of the programme
 * 
 */
void cleanup() {

    // Détacher la mémoire partagée
    detach_shared_memory();
    
    // deleting the pipes in the main now
    // // Delete the pipe
    // if (unlink(PIPE_TO_GESTION) == -1) {
    //     perror("unlink");
    //     exit(EXIT_FAILURE);
    // }
    // // Delete the pipe
    // if (unlink(PIPE_TO_FILE_MSG) == -1) {
    //     perror("unlink");
    //     exit(EXIT_FAILURE);
    // }

    printf("Clean exit.\n");
    exit(EXIT_SUCCESS); 
}


/**
 * @brief Handle the message based on the prefix and decide which function it needs to execute to send the right response
 * 
 * @param message Message received
 * @param sockfd socket UDP
 * @param servaddr struct with sock address
 * @param pipe_to_file_msg_write pipe to write
 * @param shared_memory shared memory access
 */
void handle_message(char message[MAX_MESSAGE_SIZE], int sockfd, struct sockaddr_in *servaddr, int pipe_to_file_msg_write, char *shared_memory) {

    // Extraire les 5 premiers caractères du message
    char type_msg[6]; // 5 caractères + 1 pour le caractère nul
    strncpy(type_msg, message, 5);
    type_msg[5] = '\0'; // Assurez-vous de terminer la chaîne par un caractère nul
    printf("%s\n", type_msg);

    char message_content[strlen(message) - 5]; // Subtract 5 for the first 5 characters
    strncpy(message_content, message + 5, strlen(message) - 5);
    message_content[strlen(message) - 5] = '\0'; // Null-terminate the string

    char pseudo[50] = {0};
    char password[50] = {0}; 

    // Extract pseudo and password from the message_content
    sscanf(message_content, "%49[^#]#%49s", pseudo, password);

    // Switch en fonction des 5 premiers caractères
    switch (type_msg[0]) {
        case 'A':
            if (strcmp(type_msg, "A_ACC") == 0) {

                if (pseudo_not_in_shared_memory(pseudo)){

                    // Envoyer au serveur UDP
                    send_udp_message(message, sockfd, servaddr);

                    // Attendre la réponse du serveur UDP et la renvoyer sur le pipe file_msg
                    char *response = receive_udp_response(sockfd, servaddr);

                    if (strcmp(response, "Success") == 0) {
                        // Ajouter le pseudo à la mémoire partagée
                        add_to_shared_memory(pseudo);
                    }

                    send_response_pipe(pipe_to_file_msg_write, response);
                }
                else{
                    send_response_pipe(pipe_to_file_msg_write, "Failed");
                }
            } else {
                printf("Unknown message type: %s\n", type_msg);
            }
            break;
        case 'C':
        case 'D':
            if (strcmp(type_msg, "D_ACC") == 0 || strcmp(type_msg, "C_ACC") == 0) {
                // Envoyer le message UDP au serveur
                send_udp_message(message, sockfd, servaddr);

                // Attendre la réponse du serveur UDP et la renvoyer sur le pipe file_msg
                char *response = receive_udp_response(sockfd, servaddr);
                send_response_pipe(pipe_to_file_msg_write, response);
            } else {
                printf("Unknown message type: %s\n", type_msg);
            }
            break;
            
        case 'L':
            if (strcmp(type_msg, "L_ACC") == 0) {
                // Logout
                // Remove pseudo from shared memory
                remove_from_shared_memory(pseudo);

                char response[8] = "Success";
                response[8] = '\0';
                send_response_pipe(pipe_to_file_msg_write, response);
            } else {
                printf("Unknown message type: %s\n", type_msg);
            }
            break;
        default:
            printf("Unknown message type: %s\n", type_msg);
            break;
    }
}

int main() {
    printf("Welcome to gest_req\n");

    // Register signal handler for SIGINT (Ctrl+C)
    if (signal(SIGINT, cleanup) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    // Register signal handler for SIGSEGV
    if (signal(SIGSEGV, cleanup) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    // Création du socket UDP
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Adresse IP du client RMI

    // Ouvrir le pipe gestion en lecture
    int pipe_to_gestion_read = open(PIPE_TO_GESTION, O_RDWR);
    if (pipe_to_gestion_read == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }


    // Ouvrir le nouveau pipe en écriture
    int pipe_to_file_msg_write = open(PIPE_TO_FILE_MSG, O_RDWR);
    if (pipe_to_file_msg_write == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Créer l'espace de mémoire partagée
    create_shared_memory();

    // Boucle de lecture des messages depuis le pipe gestion

    while (1) {
        char message[MAX_MESSAGE_SIZE] = {0};
        // Buffer pour stocker les messages

        // Lire le message depuis le pipe gestion
        ssize_t bytes_read = read(pipe_to_gestion_read, message, sizeof(message));
        if (bytes_read == -1) {
            perror("Erreur lors de la lecture depuis le pipe gestion");
            exit(EXIT_FAILURE);
        } else if (bytes_read == 0) {
            printf("Aucune donnée reçue du pipe gestion.\n");
            continue;
        }

        // Ajouter un caractère nul à la fin pour former une chaîne valide
        message[bytes_read] = '\0';

        handle_message(message, sockfd, &servaddr, pipe_to_file_msg_write, shared_memory);
    }

    // Détacher l'espace de mémoire partagée
    detach_shared_memory();

    // Fermer les sockets et pipes gestion
    close(sockfd);
    close(pipe_to_gestion_read);
    close(pipe_to_file_msg_write);  // Fermer le pipe vers file_msg
    // if (unlink(PIPE_TO_GESTION) == -1) {
    //     perror("Error unlinking pipe");
    //     return -1;
    // }
    // if (unlink(PIPE_TO_FILE_MSG) == -1) {
    //     perror("Error unlinking pipe");
    //     return -1;
    // }

    return 0;
}
