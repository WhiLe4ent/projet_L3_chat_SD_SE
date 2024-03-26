#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
void viderBuffer()
{
    int c = 0;
    while (c != '\n' && c != EOF)
    {
        c = getchar();
    }
}

int main (int argc, char const *argv[]) {

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *message = "Message reçu par le serveur";

    // Création du socket serveur
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configuration des options du socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    // Attachement du socket à un port donné
    if (bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Mise en écoute du socket
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Acceptation de la connexion entrante
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    while(1){
        
        // Lecture du message envoyé par le client
        memset(buffer, 0, 1024);
        valread = read( new_socket , buffer, 1024);
        printf("%s\n",buffer );

        // Envoi d'une réponse au client
        send(new_socket , message , strlen(message) , 0 );
        printf("Message envoyé au client\n");
    }
    
    return 0;
}
