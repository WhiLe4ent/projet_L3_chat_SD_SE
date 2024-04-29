#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>


#define PIPE_TO_GESTION "./pipe_to_gestion"
#define PIPE_GEST_TO_FILE_MSG "./pipe_gest_to_file_msg"
#define PIPE_COM_TO_FILE_MSG "./pipe_com_to_file_msg"
#define PIPE_TO_COM "./pipe_to_com"


int main() {
    // Compiler les fichiers source
    system("gcc ./server_com/server.c -o ./server_com/server -pthread");
    system("gcc ./file_message/file_msg.c -o ./file_message/file_msg -pthread");
    system("gcc ./gestion_requete/gest_req.c -o ./gestion_requete/gest_req -pthread");

    // Lancer les programmes dans des processus enfants
    pid_t pid_server = fork();
    if (pid_server == 0) {
        // Processus enfant pour le serveur
        execlp("gnome-terminal", "gnome-terminal", "--", "./server_com/server", NULL);
        perror("Erreur lors du démarrage du serveur");
        exit(EXIT_FAILURE);
    } else if (pid_server < 0) {
        perror("Erreur lors de la création du processus serveur");
        exit(EXIT_FAILURE);
    }

    pid_t pid_file_msg = fork();
    if (pid_file_msg == 0) {
        // Processus enfant pour file_msg
        execlp("gnome-terminal", "gnome-terminal", "--", "./file_message/file_msg", NULL);
        perror("Erreur lors du démarrage de file_msg");
        exit(EXIT_FAILURE);
    } else if (pid_file_msg < 0) {
        perror("Erreur lors de la création du processus file_msg");
        exit(EXIT_FAILURE);
    }

    pid_t pid_gest_req = fork();
    if (pid_gest_req == 0) {
        // Processus enfant pour gest_req
        execlp("gnome-terminal", "gnome-terminal", "--", "./gestion_requete/gest_req", NULL);
        perror("Erreur lors du démarrage de gest_req");
        exit(EXIT_FAILURE);
    } else if (pid_gest_req < 0) {
        perror("Erreur lors de la création du processus gest_req");
        exit(EXIT_FAILURE);
    }

    // Attendre la fin des processus enfants
    waitpid(pid_server, NULL, 0);
    waitpid(pid_file_msg, NULL, 0);
    waitpid(pid_gest_req, NULL, 0);

    // Supprimer les pipes

    if (unlink(PIPE_TO_GESTION) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }
    if (unlink(PIPE_GEST_TO_FILE_MSG) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }
    if (unlink(PIPE_COM_TO_FILE_MSG) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }
    if (unlink(PIPE_TO_COM) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }

    printf("Pipes unlinked successfully.\n");

    return 0;
}
