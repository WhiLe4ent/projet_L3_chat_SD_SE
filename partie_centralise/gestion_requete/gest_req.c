#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 4003
#define MAX_MESSAGE_SIZE 1024

void send_udp_message(const char *message) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Création du socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Remplissage des informations du serveur
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Adresse IP du client RMI

    // Envoi du message
    if (sendto(sockfd, (const char *)message, strlen(message), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Erreur lors de l'envoi du message");
        exit(EXIT_FAILURE);
    }

    printf("Message envoyé avec succès : %s\n", message);

    close(sockfd);
}

int main() {
    // Boucle de lecture des messages (dummy pour l'instant)
    char message[MAX_MESSAGE_SIZE];
    while (fgets(message, MAX_MESSAGE_SIZE, stdin) != NULL) {
        // Vérifier le préfixe du message
        if (strncmp(message, "L_USE", 5) == 0) {
            printf("Liste des utilisateurs demandée.\n");
            // Code pour traiter la demande de liste des utilisateurs
        } else {
            // Envoi du message UDP au ClientRMI
            send_udp_message(message);
        }
    }

    return 0;
}
