#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <fcntl.h> // Pour les constantes de mode d'ouverture des pipes
#include <sys/stat.h> // Pour la fonction mkfifo

#define PIPE_GESTION "../file_gestion"
#define MAX_MESSAGE_SIZE 1024
#define PORT 4003

void send_udp_message(const char *message, int sockfd, struct sockaddr_in *servaddr) {
    // Envoi du message
    if (sendto(sockfd, (const char *)message, strlen(message), 0, (const struct sockaddr *)servaddr, sizeof(*servaddr)) < 0) {
        perror("Erreur lors de l'envoi du message");
        exit(EXIT_FAILURE);
    }

    printf("Message envoyé avec succès : %s\n", message);
}

void receive_udp_response(int pipe_gestion_write, int sockfd, struct sockaddr_in *servaddr) {
    struct sockaddr_in cliaddr;
    socklen_t len;
    char buffer[MAX_MESSAGE_SIZE];

    memset(&cliaddr, 0, sizeof(cliaddr));

    while (1) {
        len = sizeof(cliaddr);

        // Réception du message
        ssize_t n = recvfrom(sockfd, (char *)buffer, MAX_MESSAGE_SIZE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            perror("Erreur lors de la réception du message");
            exit(EXIT_FAILURE);
        }

        buffer[n] = '\0';
        printf("Réponse du serveur UDP : %s\n", buffer);

        // Envoyer la réponse du serveur UDP sur le pipe gestion
        ssize_t bytes_written = write(pipe_gestion_write, buffer, strlen(buffer));
        if (bytes_written == -1) {
            perror("Erreur lors de l'écriture sur le pipe gestion");
            exit(EXIT_FAILURE);
        }
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

    // Buffer pour stocker les messages
    char message[MAX_MESSAGE_SIZE];

    // Boucle de lecture des messages depuis le pipe gestion
    while (1) {
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

        // Envoyer le message UDP au serveur
        send_udp_message(message, sockfd, &servaddr);

        // Attendre la réponse du serveur UDP et la renvoyer sur le pipe gestion
        receive_udp_response(pipe_gestion_write, sockfd, &servaddr);
    }

    // Fermer les sockets et pipes gestion
    close(sockfd);
    close(pipe_gestion_read);
    close(pipe_gestion_write);

    return 0;
}
