#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>

#define PIPE_TO_GESTION "./pipe_to_gestion"
#define PIPE_GEST_TO_FILE_MSG "./pipe_gest_to_file_msg"
#define PIPE_COM_TO_FILE_MSG "./pipe_com_to_file_msg"
#define PIPE_TO_COM "./pipe_to_com"

#define NUM_THREADS 10

int message_pending = 0;
int pipe_to_gestion_write, pipe_gest_to_file_msg_read, pipe_com_to_file_msg_read, pipe_to_com_write;


sem_t semaphore;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Structure pour stocker les informations d'un message
typedef struct {
    char *message;
    char tid_s[20];
} MessageInfo;

void cleanup() {
    // Code de nettoyage à exécuter avant la sortie


    // Détruire le sémaphore
    sem_destroy(&semaphore);

    // Détruire le mutex
    pthread_mutex_destroy(&mutex);

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
    exit(EXIT_FAILURE);
}

// Fonction exécutée par chaque thread pour traiter les messages
void* message_handler(void* arg) {
    MessageInfo* msg_info = (MessageInfo*)arg;


    pthread_mutex_lock(&mutex);

    // Attente pour acquérir le sémaphore
    sem_wait(&semaphore);



    printf("On attend un peu dans le thread %s \n", msg_info->message);
    printf("On attend un peu dans le thread %s \n", msg_info->tid_s);
    sleep(5);

    // Envoyer le message au pipe gestion
    printf("Sending message to gestion pipe: %s\n", msg_info->message);
    if (write(pipe_to_gestion_write, msg_info->message, strlen(msg_info->message)) == -1) {
        perror("write to gestion pipe");
        exit(EXIT_FAILURE);
    }

    // Attendre la réponse du pipe gestion
    char buffer[2048 + 50] = {0};
    ssize_t bytes_read;
    printf("Waiting for response from gestion pipe...\n");
    bytes_read = read(pipe_gest_to_file_msg_read, buffer, sizeof(buffer));
    if (bytes_read == -1) {
        perror("read from gestion pipe");
        exit(EXIT_FAILURE);
    } else if (bytes_read == 0) {
        printf("No data received from gestion pipe.\n");
    } else {
        // Afficher la réponse reçue
        buffer[bytes_read] = '\0';
        printf("Received response from gestion pipe: %s\n", buffer);
    }

    // Envoyer la réponse au pipe message
    printf("Thread_id : %s\n", msg_info->tid_s);

    char response[2053];
    snprintf(response, sizeof(msg_info->tid_s) + sizeof(buffer) + 2, "%s#%s", msg_info->tid_s, buffer);

    printf("Response : %s\n", response);

    printf("Sending response to com pipe...\n");
    if (write(pipe_to_com_write, response, strlen(response)) == -1) {
        perror("write to com pipe");
        exit(EXIT_FAILURE);
    }

    // Libérer le sémaphore
    sem_post(&semaphore);

    pthread_mutex_unlock(&mutex);

    // Libérer la mémoire allouée
    free(msg_info->message);
    free(msg_info);

    return NULL;
}

int main() {
    printf("Welcome to file_msg\n");



    ssize_t bytes_read;

    // Ouvrir les pipes en écriture
    pipe_to_com_write = open(PIPE_TO_COM, O_RDWR);
    if (pipe_to_com_write == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    pipe_to_gestion_write = open(PIPE_TO_GESTION, O_RDWR);
    if (pipe_to_gestion_write == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Ouvrir les pipes en lecture
    pipe_gest_to_file_msg_read = open(PIPE_GEST_TO_FILE_MSG, O_RDWR);
    if (pipe_gest_to_file_msg_read == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    pipe_com_to_file_msg_read = open(PIPE_COM_TO_FILE_MSG, O_RDWR);
    if (pipe_com_to_file_msg_read == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Définir le gestionnaire de signal pour SIGINT (Ctrl+C)
    if (signal(SIGINT, cleanup) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    printf("file_msg: Listening for messages...\n");
    
    // Initialiser le sémaphore avec une valeur initiale de 1
    sem_init(&semaphore, 0, 1);

    // Initialiser le mutex
    pthread_mutex_init(&mutex, NULL);


    while (1) {
        printf("Start of da loop\n");
        // Lire le message du pipe message
        char buffer[2048 + 50] = {0};
        bytes_read = read(pipe_com_to_file_msg_read, buffer, sizeof(buffer));
        pthread_mutex_lock(&mutex);
        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        } else if (bytes_read == 0) {
            printf("file_msg: No data received from the message pipe.\n");
            sleep(1); // Attendre un peu avant de réessayer
            continue;
        }

        // Extraire le thread_id du message
        char thread_id[20] = {0};
        char message[2053] = {0};
        strcpy(thread_id, strtok(buffer, "#"));
        strcpy(message, strtok(NULL, ""));

        pthread_t thread;

        // Allouer dynamiquement de la mémoire pour msg_info
        MessageInfo* msg_info = malloc(sizeof(MessageInfo));
        if (msg_info == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        strcpy(msg_info->tid_s, thread_id);
        // Allouer dynamiquement de la mémoire pour la chaîne message
        msg_info->message = strdup(message);
        if (msg_info->message == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }


        // Créer le thread pour traiter le message
        pthread_create(&thread, NULL, message_handler, msg_info);

        pthread_mutex_unlock(&mutex);


    }


    // Détruire le sémaphore
    sem_destroy(&semaphore);

    // Détruire le mutex
    pthread_mutex_destroy(&mutex);


    // Fermer les pipes
    close(pipe_to_gestion_write);
    close(pipe_gest_to_file_msg_read);
    close(pipe_com_to_file_msg_read);
    close(pipe_to_com_write);

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
