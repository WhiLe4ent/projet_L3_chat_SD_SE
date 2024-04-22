#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_MESSAGES 100

// Structure pour représenter un message
typedef struct {
    int id;
    char content[256]; // Contenu du message
    pthread_t sender_thread; // Identifiant du thread ayant envoyé le message
} Message;

// File de messages
Message message_queue[MAX_MESSAGES];
int front = 0;
int rear = -1;
int message_count = 0;

// Mutex pour synchroniser l'accès à la file de messages
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Condition pour attendre qu'un message soit disponible dans la file
pthread_cond_t message_available = PTHREAD_COND_INITIALIZER;

// Fonction pour ajouter un message à la file
void enqueue_message(Message msg) {
    pthread_mutex_lock(&mutex);
    if (message_count >= MAX_MESSAGES) {
        printf("File de messages pleine, impossible d'ajouter un nouveau message.\n");
        pthread_mutex_unlock(&mutex);
        return;
    }
    rear = (rear + 1) % MAX_MESSAGES;
    message_queue[rear] = msg;
    message_count++;
    printf("Message ajouté à la file. ID: %d\n", msg.id);
    pthread_cond_signal(&message_available); // Signaler qu'un message est disponible
    pthread_mutex_unlock(&mutex);
}

// Fonction pour retirer un message de la file
Message dequeue_message() {
    pthread_mutex_lock(&mutex);
    while (message_count <= 0) {
        pthread_cond_wait(&message_available, &mutex); // Attendre qu'un message soit disponible
    }
    Message msg = message_queue[front];
    front = (front + 1) % MAX_MESSAGES;
    message_count--;
    printf("Message retiré de la file. ID: %d\n", msg.id);
    pthread_mutex_unlock(&mutex);
    return msg;
}

// Fonction pour traiter un message et envoyer une réponse
void process_message(Message msg) {
    // Simuler un traitement en attente de 2 secondes
    sleep(2);
    // Simuler l'envoi d'une réponse
    printf("Réponse envoyée au thread %lu pour le message avec l'ID: %d\n", msg.sender_thread, msg.id);
}

// Fonction exécutée par les threads
void *thread_function(void *arg) {
    while (1) {
        // Attendre et récupérer un message de la file
        Message msg = dequeue_message();
        // Traiter le message et envoyer une réponse
        process_message(msg);
    }
    return NULL;
}

int main() {
    // Créer des threads pour traiter les messages
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, thread_function, NULL);
    pthread_create(&thread2, NULL, thread_function, NULL);

    // Envoyer quelques messages pour tester
    Message msg1 = {1, "Premier message", thread1};
    Message msg2 = {2, "Deuxième message", thread2};
    enqueue_message(msg1);
    enqueue_message(msg2);

    // Attendre la fin des threads
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return 0;
}
