#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> // Pour les constantes de mode d'ouverture des pipes
#include <sys/stat.h> // Pour la fonction mkfifo

#define PIPE_COM "../file_com"
#define PIPE_GESTION "../file_gestion"

int main() {
    // Créer le pipe file_gestion avec mkfifo
    if (mkfifo(PIPE_GESTION, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    int pipe_msg_read, pipe_gestion_write, pipe_gestion_read, pipe_msg_write;
    char buffer[2048 + 50]; // Taille maximale du message + taille maximale de l'ID de thread
    ssize_t bytes_read;

    // Ouvrir le pipe message en lecture
    pipe_msg_read = open(PIPE_COM, O_RDONLY);
    if (pipe_msg_read == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Ouvrir le pipe gestion en écriture
    pipe_gestion_write = open(PIPE_GESTION, O_WRONLY);
    if (pipe_gestion_write == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Ouvrir le pipe gestion en lecture
    pipe_gestion_read = open(PIPE_GESTION, O_RDONLY);
    if (pipe_gestion_read == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Ouvrir le pipe message en écriture
    pipe_msg_write = open(PIPE_COM, O_WRONLY);
    if (pipe_msg_write == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    printf("file_msg: Listening for messages...\n");

    while (1) {
        // Lire le message du pipe message
        bytes_read = read(pipe_msg_read, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        } else if (bytes_read == 0) {
            printf("file_msg: No data received from the message pipe.\n");
            sleep(1); // Attendre un peu avant de réessayer
            continue;
        }

        // Afficher le message reçu
        buffer[bytes_read] = '\0'; // Ajouter un caractère nul à la fin pour s'assurer qu'il s'agit d'une chaîne valide
        printf("file_msg: Received message from com pipe: %s\n", buffer);

        // Envoyer le message au pipe gestion
        printf("file_msg: Sending message to gestion pipe...\n");
        if (write(pipe_gestion_write, buffer, bytes_read) == -1) {
            perror("write to gestion pipe");
            exit(EXIT_FAILURE);
        }

        // Attendre la réponse du pipe gestion
        printf("file_msg: Waiting for response from gestion pipe...\n");
        bytes_read = read(pipe_gestion_read, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            perror("read from gestion pipe");
            exit(EXIT_FAILURE);
        } else if (bytes_read == 0) {
            printf("file_msg: No data received from gestion pipe.\n");
            continue;
        }

        // Afficher la réponse reçue
        buffer[bytes_read] = '\0'; // Ajouter un caractère nul à la fin pour s'assurer qu'il s'agit d'une chaîne valide
        printf("file_msg: Received response from gestion pipe: %s\n", buffer);

        // Envoyer la réponse au pipe message
        printf("file_msg: Sending response to com pipe...\n");
        if (write(pipe_msg_write, buffer, bytes_read) == -1) {
            perror("write to com pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Fermer les pipes
    close(pipe_msg_read);
    close(pipe_msg_write);
    close(pipe_gestion_read);
    close(pipe_gestion_write);

    return 0;
}
