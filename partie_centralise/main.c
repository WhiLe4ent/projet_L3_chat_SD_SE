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
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



#define ROWS 10
#define COLS 50

int shmid;


/**
 * @brief Localised a shared memory 
 * 
 */
void create_shared_memory() {
    // Générer une clé unique avec ftok
    key_t key = ftok("./memoire_partagee/mem_part.txt", 65);

    // Créer ou localiser un segment de mémoire partagée
    shmid = shmget(key, sizeof(char) * ROWS * COLS, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
}


/**
 * @brief Delete the shared memory
 * 
 */
void delete_shared_memory(){
    // Supprimer la mémoire partagée
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
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

    // mutex pour stopper l'exécution tant que tout les fichiers ne sont pas compilé
    pthread_mutex_lock(&mutex);

    // Compiling source file
    if(system("gcc ./server_com/server.c -o ./server_com/server -pthread") != 0){
        printf("erver compilation failed\n");
        sleep(1);
    }
    if(system("gcc ./file_message/file_msg.c -o ./file_message/file_msg -pthread") !=0){
           printf("file_msg compilation failed\n");
           sleep(1);
    }
    if(system("gcc ./gestion_requete/gest_req.c -o ./gestion_requete/gest_req -pthread")!= 0){
        printf("gestion_req compilation failed\n");
        sleep(1);
    }

    if (system("javac -cp /home/while4ent/.m2/repository/com/gestion_compte/gestion_compte/1.0-SNAPSHOT/gestion_compte-1.0-SNAPSHOT.jar:/home/while4ent/Bureau/fac/L3/S6/Projet/projet_L3_chat_SD_SE/partie_centralise/clientrmi/target/classes ./clientrmi/src/main/java/com/clientrmi/ClientRMI.java ") != 0) {
        perror("Erreur lors de la compilation de ClientRMI.java\n");
        sleep(1);
        exit(EXIT_FAILURE);
    }
    
    pthread_mutex_unlock(&mutex);

    char server_ip[20] = {0};
    int rmi_port, tcp_port;

    // Adresse IP du serveur RMI
    printf("Entrez l'adresse IP du serveur RMI : ");
    scanf("%19s", server_ip); 

    // Port du serveur RMI
    printf("Entrez le port du serveur RMI (1099 est conseillé): \n");
    scanf("%d", &rmi_port);

    // Port TCP
    printf("Entrez le port TCP (8080 est conseillé) : \n");
    scanf("%d", &tcp_port);

    // Lancement du serveur, file_msg et gest_req
    pthread_mutex_lock(&mutex);
    char command[200]; // Définir une taille suffisante pour la commande
    // Construction de la commande avec les arguments
    sprintf(command, "gnome-terminal -- ./server_com/server %d", tcp_port);
    if (system(command) != 0) {
        perror("Error launching server_com/server");
        exit(EXIT_FAILURE);
    }
    sprintf(command, "gnome-terminal -- ./file_message/file_msg");
    if (system(command) != 0) {
        perror("Error launching file_message/file_msg");
        exit(EXIT_FAILURE);
    }
    sprintf(command, "gnome-terminal -- ./gestion_requete/gest_req");
    if (system(command) != 0) {
        perror("Error launching gestion_requete/gest_req");
        exit(EXIT_FAILURE);
    }

    printf("Exécution de Client_RMI.java...\n");
    // Construction de la commande pour exécuter Java avec les arguments récupérés
    sprintf(command, "java -cp ./clientrmi/target/classes:/home/while4ent/.m2/repository/com/gestion_compte/gestion_compte/1.0-SNAPSHOT/gestion_compte-1.0-SNAPSHOT.jar com.clientrmi.ClientRMI %s %d", server_ip, rmi_port);
    // Exécution de la commande et vérification si l'exécution a réussi
    if (system(command) != 0) {
        perror("Erreur lors de l'exécution de ClientRMI\n");
    }
    pthread_mutex_unlock(&mutex);

    printf("C'est finis !\n");


    // Détacher la mémoire partagée
    delete_shared_memory();
    
    // Delete pipes
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

    return 0;
}