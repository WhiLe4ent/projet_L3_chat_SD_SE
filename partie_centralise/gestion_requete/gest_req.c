#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define TCP_PORT 8081
#define UDP_PORT 9090
#define BUFFER_SIZE 2048

// Fonction pour gérer les demandes en fonction du type de message
void handle_message(const char *message) {
    // Analyser le type de message
    if (strcmp(message, "A_ACC") == 0) {
        // Traitement pour "A_ACC" (Authentication)
        printf("Received authentication request\n");
        // Envoyer une demande au serveur UDP si nécessaire
    } else if (strcmp(message, "C_ACC") == 0) {
        // Traitement pour "C_ACC" (Create Account)
        printf("Received account creation request\n");
        // Envoyer une demande au serveur UDP si nécessaire
    } else if (strcmp(message, "D_ACC") == 0) {
        // Traitement pour "D_ACC" (Delete Account)
        printf("Received account deletion request\n");
        // Envoyer une demande au serveur UDP si nécessaire
    } else if (strcmp(message, "L_USE") == 0) {
        // Traitement pour "L_USE" (List Users)
        printf("Received request to list users\n");
        // Envoyer une demande au serveur UDP si nécessaire
    } else {
        // Type de message inconnu
        printf("Unknown message type: %s\n", message);
    }
}

int main() {
    int tcp_socket, udp_socket;
    struct sockaddr_in tcp_address, udp_address;
    int tcp_addrlen = sizeof(tcp_address), udp_addrlen = sizeof(udp_address);
    char buffer[BUFFER_SIZE];

    // Création du socket TCP
    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse TCP
    memset(&tcp_address, 0, sizeof(tcp_address));
    tcp_address.sin_family = AF_INET;
    tcp_address.sin_addr.s_addr = INADDR_ANY;
    tcp_address.sin_port = htons(TCP_PORT);

    // Attachement du socket TCP au port
    if (bind(tcp_socket, (struct sockaddr *)&tcp_address, sizeof(tcp_address)) == -1) {
        perror("bind failed");
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    // Écoute des connexions entrantes
    if (listen(tcp_socket, 1) == -1) {
        perror("listen failed");
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    printf("TCP server listening on port %d...\n", TCP_PORT);

    // Accepte une connexion TCP entrante
    int client_socket;
    if ((client_socket = accept(tcp_socket, (struct sockaddr *)&tcp_address, (socklen_t *)&tcp_addrlen)) == -1) {
        perror("accept failed");
        close(tcp_socket);
        exit(EXIT_FAILURE);
    }

    printf("TCP connection accepted.\n");

    // Attente de messages TCP
    int recv_len;
    while ((recv_len = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[recv_len] = '\0';
        printf("Message received from TCP client: %s\n", buffer);

        // Traitement du message reçu
        handle_message(buffer);
    }

    if (recv_len == 0) {
        printf("TCP client disconnected.\n");
    } else {
        perror("recv failed");
    }

    // Fermeture des sockets
    close(client_socket);
    close(tcp_socket);

    return 0;
}
