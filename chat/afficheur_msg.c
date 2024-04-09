#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PIPE_NAME "message_pipe"

int main() {
    int pipe_fd;
    char pipe_buffer[1024];

    // Ouvrir le tube pour la lecture
    pipe_fd = open(PIPE_NAME, O_RDONLY);
    if (pipe_fd == -1) {
        perror("Échec de l'ouverture du tube");
        exit(EXIT_FAILURE);
    }

    // Indiquer la connexion dans le terminal
    printf("Afficheur de message.\n");

    // Lire à partir du tube et afficher les messages
    while (1) {
        ssize_t bytes_read = read(pipe_fd, pipe_buffer, sizeof(pipe_buffer));
        if (bytes_read == -1) {
            perror("Échec de lecture du tube");
            break;
        } else if (bytes_read == 0) {
            // Le tube est fermé
            printf("Le serveur s'est déconnecté.\n");
            break;
        } else {
            // Afficher le message
            printf("%.*s", (int)bytes_read, pipe_buffer);
        }
    }

    // Fermer le tube
    close(pipe_fd);

    return 0;
}
