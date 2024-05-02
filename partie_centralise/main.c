#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

int main() {

    // Create the named pipe ---------------------------------------------------------
    if (mkfifo(PIPE_COM_TO_FILE_MSG, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    if (mkfifo(PIPE_TO_COM, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
        // Créer les pipes
    if (mkfifo(PIPE_TO_GESTION, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    if (mkfifo(PIPE_GEST_TO_FILE_MSG, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
    create_shared_memory();


    // Compiling source file
    if(system("gcc ./server_com/server.c -o ./server_com/server -pthread") != 0){
        printf("erver compilation failed\n");
    }
    if(system("gcc ./file_message/file_msg.c -o ./file_message/file_msg -pthread") !=0){
           printf("file_msg compilation failed\n");
    }
    if(system("gcc ./gestion_requete/gest_req.c -o ./gestion_requete/gest_req -pthread")!= 0){
        printf("gestion_req compilation failed\n");
    }
    
    // wait 10 millisecondes to finish compiling
    usleep(10000); 

    int server = system("gnome-terminal -- ./server_com/server");

    // pour file_msg
    int file_msg = system("gnome-terminal -- ./file_message/file_msg");

    // pour gest_req
    int gest_req = system("gnome-terminal -- ./gestion_requete/gest_req");


    return 0;
}