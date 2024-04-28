#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <semaphore.h>

#define PIPE_COM "../file_com"
#define PIPE_GESTION "../file_gestion"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int message_pending = 0;
int pipe_gestion_read, pipe_gestion_write, pipe_com_write, pipe_com_read;

// Structure pour stocker les informations du thread
typedef struct {
    sem_t semaphore;
    int is_busy;
    char **message_queue;
    int queue_length;
} ThreadInfo;

// Structure pour stocker les informations d'un message
typedef struct {
    ThreadInfo *thread_infos;
    int tid;
    char *message;
    char tid_s[20];
} MessageInfo;

// Fonction exécutée par chaque thread pour traiter les messages
void* message_handler(void* arg) {
    MessageInfo* msg_info = (MessageInfo*)arg;

    // Attendre le signal pour commencer le traitement
    sem_wait(&msg_info->thread_infos[msg_info->tid].semaphore);

    // Envoyer le message au pipe gestion
    printf("Sending message to gestion pipe: %s\n", msg_info->message);
    if (write(pipe_gestion_write, msg_info->message, strlen(msg_info->message)) == -1) {
        perror("write to gestion pipe");
        exit(EXIT_FAILURE);
    }
    sleep(1);

    // Attendre la réponse du pipe gestion
    char buffer[2048 + 50] = {0};
    ssize_t bytes_read;
    printf("Waiting for response from gestion pipe...\n");
    bytes_read = read(pipe_gestion_read, buffer, sizeof(buffer));
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

    char response[2053] ;
    snprintf(response, sizeof(msg_info->tid_s) + sizeof(buffer) + 2 , "%s#%s", msg_info->tid_s, buffer);

    printf("Response : %s\n", response);

    printf("Sending response to com pipe...\n");
    if (write(pipe_com_write, response, strlen(response)) == -1) {
        perror("write to com pipe");
        exit(EXIT_FAILURE);
    }

    return NULL;
}



int main() {
    // Créer le pipe file_gestion avec mkfifo
    if (mkfifo(PIPE_GESTION, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;

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

    // Ouvrir le pipe message en lecture
    pipe_com_read = open(PIPE_COM, O_RDONLY);
    if (pipe_com_read == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Ouvrir le pipe message en écriture
    pipe_com_write = open(PIPE_COM, O_WRONLY);
    if (pipe_com_write == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Initialiser les sémaphores pour chaque thread
    ThreadInfo thread_infos[10]; // Ajustez la taille selon le nombre de threads
    for (int i = 0; i < 10; ++i) {
        sem_init(&thread_infos[i].semaphore, 0, 1);
        thread_infos[i].is_busy = 0;
        thread_infos[i].message_queue = NULL;
        thread_infos[i].queue_length = 0;
    }

    printf("file_msg: Listening for messages...\n");

    while (1) {
        
        // Lire le message du pipe message
        char buffer[2048 + 50] = {0};
        bytes_read = read(pipe_com_read, buffer, sizeof(buffer));
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

        // Trouver le thread disponible ou ajouter à la file d'attente
        int tid = -1;

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < 10; ++i) {
            if (!thread_infos[i].is_busy) {
                tid = i;
                thread_infos[i].is_busy = 1;
                break;
            }
        }

        if (tid == -1) {
            printf("Coucou\n");
            // Aucun thread disponible, ajouter à la file d'attente
            // Allouer de la mémoire pour le message et copier le message
            tid = 0; // Arbitrairement, vous pouvez implémenter une logique plus complexe ici
            thread_infos[tid].queue_length++;
            thread_infos[tid].message_queue = realloc(thread_infos[tid].message_queue,
                                                     thread_infos[tid].queue_length * sizeof(char*));
            thread_infos[tid].message_queue[thread_infos[tid].queue_length - 1] = malloc(strlen(message) + 1);
            strcpy(thread_infos[tid].message_queue[thread_infos[tid].queue_length - 1], message);
        }
        pthread_mutex_unlock(&mutex);

        if (tid != -1) {
            // Créer un thread pour traiter le message
            pthread_t thread;
            MessageInfo msg_info;
            msg_info.thread_infos = thread_infos;
            msg_info.tid = tid;
            strcpy( msg_info.tid_s, thread_id);
            msg_info.message = message;
            pthread_create(&thread, NULL, message_handler, &msg_info);
        }
    }

    // Fermer les pipes
    close(pipe_com_read);
    close(pipe_com_write);
    close(pipe_gestion_read);
    close(pipe_gestion_write);
    
    if (unlink(PIPE_COM) == -1) {
        perror("Error unlinking pipe");
        return -1;
    }
    
    if (unlink(PIPE_GESTION) == -1) {
        perror("Error unlinking pipe");
        return -1;
    }


    return 0;
}



