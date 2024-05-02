#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#define PIPE_TO_GESTION "./pipe_to_gestion"
#define PIPE_GEST_TO_FILE_MSG "./pipe_gest_to_file_msg"
#define PIPE_COM_TO_FILE_MSG "./pipe_com_to_file_msg"
#define PIPE_TO_COM "./pipe_to_com"

#define ROWS 10
#define COLS 50

void create_shared_memory() {
    // Générer une clé unique avec ftok
    key_t key = ftok("./memoire_partagee/mem_part.txt", 65);

    // Créer ou localiser un segment de mémoire partagée
    int shmid = shmget(key, sizeof(char) * ROWS * COLS, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
}

void *server_thread(void *arg) {
    system("gnome-terminal -- ./server_com/server");
    pthread_exit(NULL);
}

void *file_msg_thread(void *arg) {
    system("gnome-terminal -- ./file_message/file_msg");
    pthread_exit(NULL);
}

void *gest_req_thread(void *arg) {
    system("gnome-terminal -- ./gestion_requete/gest_req");
    pthread_exit(NULL);
}

int main() {
    pthread_t server_tid, file_msg_tid, gest_req_tid;

    // Create the named pipes
    if (mkfifo(PIPE_COM_TO_FILE_MSG, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    if (mkfifo(PIPE_TO_COM, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    if (mkfifo(PIPE_TO_GESTION, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    if (mkfifo(PIPE_GEST_TO_FILE_MSG, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    create_shared_memory();

    // Create server thread
    if (pthread_create(&server_tid, NULL, server_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Create file_msg thread
    if (pthread_create(&file_msg_tid, NULL, file_msg_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Create gest_req thread
    if (pthread_create(&gest_req_tid, NULL, gest_req_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Wait for threads to finish
    pthread_join(server_tid, NULL);
    pthread_join(file_msg_tid, NULL);
    pthread_join(gest_req_tid, NULL);

    return 0;
}