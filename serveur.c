#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define IP_ADRESSE_INTERFACE_SERVEUR "192.168.56.1"  // Adresse IP du serveur 127.0.0.1
#define PORT 8080  // Le port sur lequel le serveur écoute
#define MAX_CLIENTS 10  // Le nombre maximum de clients dans la file d'attente
#define ARRAY_SIZE (1<<10)  // Taille du tableau du serveur (2^20 éléments)
#define CHUNK_SIZE 1024  // Taille du bloc pour la réception (1024 octets)
long long totalOccurence=0;
double maxOccurenceP=0.0;
double minOccurenceP=0.0;


int find_max(int *data, int data_size) {
    int max_value = data[0];  // Initialiser max_value avec le premier élément du tableau

    for (int i = 1; i < data_size; i++) {
        if (data[i] > max_value) {
            max_value = data[i];
        }
    }

    return max_value;
}

int find_min(int *data, int data_size) {
    int min_value = data[0];  // Initialiser min_value avec le premier élément du tableau

    for (int i = 1; i < data_size; i++) {
        if (data[i] < min_value) {
            min_value = data[i];
        }
    }

    return min_value;
}


int *server_array;  // Tableau du serveur (dynamique)
pthread_mutex_t server_array_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex pour protéger l'accès au tableau

// Compteur pour savoir combien de clients ont été traités
int clients_processed = 0;
pthread_mutex_t clients_processed_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex pour protéger le compteur

// Structure pour stocker les informations des clients
typedef struct {
    int socket;
} client_info;

void generate_filename_with_counter(int client_number, char *filename, size_t size) {
    snprintf(filename, size, "donnees_ordinateur_%d.csv", client_number);
}

// Fonction pour créer un fichier CSV et y enregistrer les données
void create_data_file(int *data, int data_size) {
    char filename[256];

    // Créer un nom de fichier unique basé sur le compteur des clients traités
    generate_filename_with_counter(clients_processed, filename, sizeof(filename));

    // Ouvrir le fichier en mode écriture
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Erreur lors de la création du fichier");
        return;
    }

    // Enregistrer les données dans le fichier CSV
    fprintf(file, "Index,Pourcentage\n");  // Entête du CSV

    for (int i = 0; i < data_size; i++) {
        double percentage = ((double)data[i] / totalOccurence) * 100;  // Calcul du pourcentage
        fprintf(file, "%d,%.8f\n", i, percentage);  // Valeur (index) et pourcentage
    }


    // Fermer le fichier après l'enregistrement
    fclose(file);

    printf("Données sauvegardées dans le fichier %s\n", filename);
}

void print_server_array() {
    printf("Tableau du serveur :\n");
    for (int i = 0; i < 4; i++) {  // Affiche les 10 premiers éléments pour la vérification
        printf("%d ", server_array[i]);
    }
    printf("\n");
}

void *process_client(void *arg) {
    client_info *client_data = (client_info *)arg;
    int new_socket = client_data->socket;
    free(client_data);

    int received_size = 0;
    int number;

    // Créer un tableau temporaire pour recevoir les données du client
    int *client_array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    if (client_array == NULL) {
        perror("Erreur d'allocation mémoire pour le tableau client");
        return NULL;
    }

    while (1) {
        // Recevoir un tableau du client
        received_size = 0;
        while (received_size < ARRAY_SIZE * sizeof(int)) {
            int chunk_size = ARRAY_SIZE * sizeof(int) - received_size > CHUNK_SIZE ? CHUNK_SIZE : ARRAY_SIZE * sizeof(int) - received_size;
            int valread = read(new_socket, client_array + received_size / sizeof(int), chunk_size);
            if (valread <= 0) {
                if (valread == 0) {
                    printf("Le client a fermé la connexion.\n");
                } else {
                    perror("Erreur lors de la lecture des données");
                }
                break;
            }
            received_size += valread;
        }

        // Si la réception du tableau est correcte
        if (received_size == ARRAY_SIZE * sizeof(int)) {
            printf("Tableau du client reçu avec succès.\n");

            // Verrouiller le tableau avant de le modifier
            pthread_mutex_lock(&server_array_mutex);

            // Effectuer la somme du tableau client avec celui du serveur
            for (int i = 0; i < ARRAY_SIZE; i++) {
                server_array[i] += client_array[i];  // Additionner les valeurs du tableau client au tableau serveur
                totalOccurence += client_array[i];  // Calcul du total des occurrences

            }

            maxOccurenceP = (find_max(server_array,ARRAY_SIZE) / totalOccurence)*100 ;
            minOccurenceP = (find_min(server_array,ARRAY_SIZE) / totalOccurence)*100 ;



            // Sauvegarder les données dans un fichier avec un nom unique
            create_data_file(server_array, ARRAY_SIZE);

            // Déverrouiller le tableau
            pthread_mutex_unlock(&server_array_mutex);
            // Envoyer une confirmation au client
            number = 1;  // Confirmation de la bonne réception
            if (send(new_socket, &number, sizeof(number), 0) < 0) {
                perror("Erreur lors de l'envoi de la confirmation");
            } else {
                printf("Confirmation envoyée au client : %d\n", number);
            }
        } else {
            printf("Erreur dans la réception des données.\n");
            break;
        }

        print_server_array();
    }

    // Libérer la mémoire allouée pour le tableau client
    free(client_array);

    // Fermer la connexion avec le client
    close(new_socket);
    printf("Connexion fermée avec le client\n");

    return NULL;
}


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Allouer dynamiquement le tableau du serveur
    server_array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    if (server_array == NULL) {
        perror("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE);
    }

    // 1. Création de la socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Échec de la création de la socket");
        exit(EXIT_FAILURE);
    }
    printf("Socket créée avec succès\n");

    // 2. Préparer l'adresse du serveur
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(IP_ADRESSE_INTERFACE_SERVEUR);
    address.sin_port = htons(PORT);

    // 3. Associer la socket à l'adresse et au port spécifiés
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Échec du bind");
        exit(EXIT_FAILURE);
    }
    printf("Bind réussi, serveur prêt à écouter\n");

    // 4. Mettre la socket en mode écoute pour accepter les connexions
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Échec de l'écoute");
        exit(EXIT_FAILURE);
    }
    printf("Serveur en écoute sur le port %d...\n", PORT);

    while (1) {
        // 5. Accepter une connexion d'un client
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Échec de l'acceptation de la connexion");
            continue;
        }
        clients_processed++;
        printf("Connexion acceptée, client connecté\n");

        // 6. Créer un thread pour traiter le client
        pthread_t client_thread;
        client_info *client_data = (client_info *)malloc(sizeof(client_info));
        if (client_data == NULL) {
            perror("Erreur d'allocation mémoire pour les données du client");
            continue;
        }
        client_data->socket = new_socket;

        // Créer le thread pour traiter ce client
        if (pthread_create(&client_thread, NULL, process_client, (void *)client_data) != 0) {
            perror("Erreur lors de la création du thread");
            free(client_data);
        } else {
            // Détacher le thread pour qu'il se termine indépendamment
            pthread_detach(client_thread);
        }

        // Vérifier si tous les clients ont été traités et afficher le tableau final
        pthread_mutex_lock(&clients_processed_mutex);
        if (clients_processed == MAX_CLIENTS) {
            // Tous les clients ont été traités, afficher le tableau final
            printf("Tous les clients ont terminé, tableau final du serveur :\n");
            print_server_array();
        }
        pthread_mutex_unlock(&clients_processed_mutex);
    }

    // 7. Fermer la socket serveur
    close(server_fd);
    printf("Serveur fermé\n");

    // Libérer la mémoire allouée pour le tableau du serveur
    free(server_array);

    return 0;
}
