#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pipe_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int pipe_fd;
    char pipe_buffer[2048+5];

    // Open the pipe for reading
    pipe_fd = open(argv[1], O_RDONLY);
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    // Indiquer la connexion dans le terminal
    printf("Afficheur de message du client %s\n", argv[1]);

    // Lire à partir du tube et afficher les messages
    while (1) {
        size_t bytes_read = read(pipe_fd, pipe_buffer, sizeof(pipe_buffer));
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

    // // Ajouter une pause pour maintenir le terminal ouvert
    // printf("Appuyez sur Entrée pour quitter...\n");
    // getchar();
    
    return 0;
}
