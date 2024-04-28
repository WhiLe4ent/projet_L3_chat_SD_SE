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

#define PIPE_GESTION "../file_gestion"
#define MAX_MESSAGE_SIZE 2053
#define PORT 4003

#define ROWS 10
#define COLS 50

int next_index = 0; // Global variable to track the next available index in shared memory

void create_shared_memory(char **shared_memory) {
    // Générer une clé unique avec ftok
    key_t key = ftok("../memoire_partagee/mem_part.txt", 65);

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

void add_to_shared_memory(char *shared_memory, const char *pseudo) {
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

void remove_from_shared_memory(char *shared_memory, const char *pseudo) {
    // Parcourir la mémoire partagée pour trouver le pseudonyme et le remplacer par une chaîne vide
    for (int i = 0; i < ROWS; i++) {
        char *current_pseudo = shared_memory + i * COLS;
        if (strcmp(current_pseudo, pseudo) == 0) {
            // Trouvé le pseudonyme, le remplacer par une chaîne vide
            strcpy(current_pseudo, "");
            printf("Pseudo '%s' removed from shared memory.\n", pseudo);
            return;
        }
    }
    // Pseudonyme non trouvé
    printf("Pseudo '%s' not found in shared memory.\n", pseudo);
}



void send_udp_message(const char *message, int sockfd, struct sockaddr_in *servaddr) {
    // Envoi du message
    if (sendto(sockfd, (const char *)message, strlen(message), 0, (const struct sockaddr *)servaddr, sizeof(*servaddr)) < 0) {
        perror("Erreur lors de l'envoi du message");
        exit(EXIT_FAILURE);
    }

    printf("Message envoyé avec succès : %s\n", message);
}

char *receive_udp_response(int sockfd, struct sockaddr_in *servaddr) {
    struct sockaddr_in cliaddr;
    socklen_t len;
    static char buffer[MAX_MESSAGE_SIZE];  // Utilisation d'une variable statique pour éviter la fuite de mémoire

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

    return buffer;  // Retourner directement le pointeur vers la variable statique
}

void send_response_pipe(int pipe_gestion_write, char *response) {
    // Envoyer la réponse du serveur UDP sur le pipe gestion
    ssize_t bytes_written = write(pipe_gestion_write, response, strlen(response));
    if (bytes_written == -1) {
        perror("Erreur lors de l'écriture sur le pipe gestion");
        exit(EXIT_FAILURE);
    }
    printf("Done\n");
    sleep(1);
}






void handle_message(char message[MAX_MESSAGE_SIZE], int sockfd, struct sockaddr_in *servaddr, int pipe_gestion_write, char *shared_memory) {
    // Extraire les 5 premiers caractères du message
    char type_msg[6]; // 5 caractères + 1 pour le caractère nul
    strncpy(type_msg, message, 5);
    type_msg[5] = '\0'; // Assurez-vous de terminer la chaîne par un caractère nul
    printf("%s\n", type_msg);

    char message_content[strlen(message) - 5]; // Subtract 5 for the first 5 characters
    strncpy(message_content, message + 5, strlen(message) - 5);
    message_content[strlen(message) - 5] = '\0'; // Null-terminate the string

    char pseudo[50] = {0};
    char password[50] = {0}; // Adjust size as needed

    // Extract pseudo and password from the message_content
    sscanf(message_content, "%49[^#]#%49s", pseudo, password);

    // Switch en fonction des 5 premiers caractères
    switch (type_msg[0]) {
        case 'A':
            if (strcmp(type_msg, "A_ACC") == 0) {
                // Envoyer au serveur UDP
                send_udp_message(message, sockfd, servaddr);

                // Attendre la réponse du serveur UDP et la renvoyer sur le pipe gestion
                char *response = receive_udp_response(sockfd, servaddr);

                if (strcmp(response, "Success") == 0) {
                    // Ajouter le pseudo à la mémoire partagée
                    add_to_shared_memory(shared_memory, pseudo);
                }

                send_response_pipe(pipe_gestion_write, response);
            } else {
                printf("Unknown message type: %s\n", type_msg);
            }
            break;
        case 'D':
            if (strcmp(type_msg, "D_ACC") == 0) {
                // Envoyer le message UDP au serveur
                send_udp_message(message, sockfd, servaddr);

                // Attendre la réponse du serveur UDP et la renvoyer sur le pipe gestion
                char *response = receive_udp_response(sockfd, servaddr);
                send_response_pipe(pipe_gestion_write, response);
            } else {
                printf("Unknown message type: %s\n", type_msg);
            }
            break;
        case 'C':
            if (strcmp(type_msg, "C_ACC") == 0) {
                // Envoyer le message UDP au serveur
                send_udp_message(message, sockfd, servaddr);

                // Attendre la réponse du serveur UDP et la renvoyer sur le pipe gestion
                char *response = receive_udp_response(sockfd, servaddr);
                send_response_pipe(pipe_gestion_write, response);
            } else {
                printf("Unknown message type: %s\n", type_msg);
            }
            break;
        case 'L':
            if (strcmp(type_msg, "L_ACC") == 0) {
                // Logout
                // Remove pseudo from shared memory
                remove_from_shared_memory(shared_memory, pseudo);

                char response[8] = "Success" ;
                response[8] = '\0' ;
                send_response_pipe(pipe_gestion_write, response);
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
    int pipe_gestion_read = open(PIPE_GESTION, O_RDONLY);
    if (pipe_gestion_read == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Ouvrir le pipe gestion en écriture
    int pipe_gestion_write = open(PIPE_GESTION, O_WRONLY);
    if (pipe_gestion_write == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Créer l'espace de mémoire partagée
    char *shared_memory;
    create_shared_memory(&shared_memory);

    // Boucle de lecture des messages depuis le pipe gestion
    while (1) {
        // Buffer pour stocker les messages
        char message[MAX_MESSAGE_SIZE] = {0};
        sleep(1); // en attendant, faudra ajouter un mutex sur le pipe d'un côté ou de l'autre.
        // Lire le message depuis le pipe gestion
        ssize_t bytes_read = read(pipe_gestion_read, message, sizeof(message));
        if (bytes_read == -1) {
            perror("Erreur lors de la lecture depuis le pipe gestion");
            exit(EXIT_FAILURE);
        } else if (bytes_read == 0) {
            printf("Aucune donnée reçue du pipe gestion.\n");
            continue;
        }

        // Ajouter un caractère nul à la fin pour former une chaîne valide
        message[bytes_read] = '\0';

        handle_message(message, sockfd, &servaddr, pipe_gestion_write, shared_memory);
        
    }

    // Détacher l'espace de mémoire partagée
    detach_shared_memory(shared_memory);

    // Fermer les sockets et pipes gestion
    close(sockfd);
    close(pipe_gestion_read);
    close(pipe_gestion_write);
    if (unlink(PIPE_GESTION) == -1) {
        perror("Error unlinking pipe");
        return -1;
    }

    return 0;
}
