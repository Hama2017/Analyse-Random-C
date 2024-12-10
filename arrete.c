#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#define ADRESSE_IP_INTERFACE_SERVEUR "127.0.0.1"  // Adresse IP du serveur
#define PORT 8080  // Le port sur lequel le serveur écoute
#define TAILLE_TABLEAU (1<<28)  // Taille du tableau à envoyer (2^28 éléments)
#define NBR_PROCESSUS 6         // Nombre de processus
#define NBR_CYCLES 10             // Nombre de cycles
#define NBR_RANDOMS (1000000000L) // Nombres aléatoires par cycle (10 milliard)
#define SHM_KEY 0x12          // Clé pour la mémoire partagée
#define K 1024       // Taille d'un bloc pour le pliage (pour generer le csv Index,Occurence avoir une allure de la courbe )
#define NBR_CASES 4800        // Nombre de case pour calculer la moyenne (MaxO - Min0)

struct sembuf bloquer_semaphore = {0, -1, 0};   // Décrémenter sémaphore (verrouiller)
struct sembuf debloquer_semaphore = {0, 1, 0}; // Incrémenter sémaphore (déverrouiller)

double moyenne=0; // Moyenne (Max - Min) / NBR_CASES
long long maxOccurence=0; // l'occurence la plus grande
long long minOccurence=0; // l'occurence la plus petite
long long nbr_cases_theorique  = 240000000000LL;  // Nombre de case theorique pour calculer la moyenne (MaxO - Min0) 2400 milliard

// FONCTIONS UTILISES

// Fonction pour generer une graine le plus unique possible
unsigned int generer_graine() {
    unsigned int graine = 0;

    graine = (unsigned int)(time(NULL) ^ getpid());

    return graine;
}

// Fonction pour générer les nombres aléatoires et synchroniser
void generer_randoms(int process_id, int *tableau_IPC, int sem_id) {
    int *tableau_local = malloc(TAILLE_TABLEAU * sizeof(int));
    if (tableau_local == NULL) {
        perror("Erreur d'allocation mémoire pour le tableau local");
        exit(1);
    }

    memset(tableau_local, 0, TAILLE_TABLEAU * sizeof(int));

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int j = 0; j < NBR_CYCLES; ++j) {

        for (long i = 0; i < NBR_RANDOMS; i++) {
            int rand_num = random() % TAILLE_TABLEAU;
            tableau_local[rand_num]++;
        }

        // Synchronisation avec le tableau global
        semop(sem_id, &bloquer_semaphore, 1); // Verrouillage
        for (int i = 0; i < TAILLE_TABLEAU; i++) {
            tableau_IPC[i] += tableau_local[i];
        }
        semop(sem_id, &debloquer_semaphore, 1); // Déverrouillage

        printf("Processus %d : synchronisation cycle %d terminée\n", process_id, j + 1);

        memset(tableau_local, 0, TAILLE_TABLEAU * sizeof(int));
    }

    gettimeofday(&end, NULL);

    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("Processus %d : génération et synchronisation terminées en %.2f secondes\n", process_id, elapsed_time);

    free(tableau_local);
}

// Fonction pour créer un fichier CSV et y enregistrer les données du tableau IPC (Index,Occurence)
void generer_csv_index_occurence(int *tableau, int taille) {
    char nom_fichier[256];
    // Créer un nom de fichier unique basé sur le compteur des clients traités
    snprintf(nom_fichier, sizeof(nom_fichier), "donnees_analyse.csv");


    // Ouvrir le fichier en mode écriture
    FILE *file = fopen(nom_fichier, "w");
    if (file == NULL) {
        perror("Erreur lors de la création du fichier");
        return;
    }

    // Enregistrer les données dans le fichier CSV
    fprintf(file, "Index,Occurence\n");  // Entête du CSV

    for (unsigned int i = 0; i < taille; i++) {
        fprintf(file, "%d,%d\n", i, tableau[i]);  // Valeur (index) et pourcentage
    }


    // Fermer le fichier après l'enregistrement
    fclose(file);

    printf("Données sauvegardées dans le fichier %s\n", nom_fichier);
}

// Fonction pour effectuer le pliage
int* plier_tableau( int* tableau,  int taille,  int taille_bloc,  int* nouvelle_taille) {
    // Calcul de la taille du tableau réduit
    *nouvelle_taille = taille / taille_bloc;
    int* tableau_reduit = malloc(*nouvelle_taille * sizeof(int));
    if (!tableau_reduit) {
        perror("Erreur d'allocation mémoire pour le tableau réduit");
        return NULL;
    }

    // Pliage : somme des valeurs dans des blocs de taille taille_bloc
    for (unsigned int i = 0; i < *nouvelle_taille; i++) {
        unsigned int somme = 0;
        for (unsigned int j = 0; j < taille_bloc; j++) {
            somme += tableau[i * taille_bloc + j];
        }
        tableau_reduit[i] = somme; // Stocke la somme ou une autre statistique
    }

    return tableau_reduit;
}

// Fonction pour trouver le maxOccurence
long long trouver_maxOccurence(int *tableau, int taille) {
    // Initialisation de la valeur maximale avec le premier élément du tableau
    long long valeur_max = tableau[0];

    // Parcourir le tableau à partir du deuxième élément
    for (int i = 1; i < taille; i++) {
        // Si la valeur actuelle est plus grande que la valeur maximale trouvée jusqu'à présent
        if (tableau[i] > valeur_max) {
            // Mettre à jour la valeur maximale
            valeur_max = tableau[i];
        }
    }

    return valeur_max;
}

// Fonction pour trouver le minOccurence
long long trouver_minOccurence(int *tableau, int taille) {
    // Initialisation de la valeur minimale avec le premier élément du tableau
    long long valeur_min = tableau[0];

    // Parcourir le tableau à partir du deuxième élément
    for (int i = 1; i < taille; i++) {
        // Si la valeur actuelle est plus petite que la valeur minimale trouvée jusqu'à présent
        if (tableau[i] < valeur_min) {
            // Mettre à jour la valeur minimale
            valeur_min = tableau[i];
        }
    }

    return valeur_min;
}


int main() {


    // PARTIE 2 - Création et configuration de la socket

    int sock = 0;  // Déclaration de la variable pour la socket
    struct sockaddr_in serv_addr;  // Déclaration de la structure pour les informations de l'adresse du serveur

    // Créer une nouvelle socket de communication réseau
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur de création de la socket");
        return -1;
    }

    // Initialiser les informations de l'adresse du serveur
    serv_addr.sin_family = AF_INET;  // Spécifier le domaine d'adresse IPv4
    serv_addr.sin_port = htons(PORT);  // Convertir la valeur du port de l'ordre réseau à l'ordre machine pour le réseau (Convertion en BigEndian)


    // Conversion de l'adresse IP en format binaire
    if (inet_pton(AF_INET, ADRESSE_IP_INTERFACE_SERVEUR, &serv_addr.sin_addr) <= 0) {
        perror("Adresse du serveur invalide");
        return -1;
    }

    // Connexion au serveur
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Échec de la connexion au serveur");
        return -1;
    }

    printf("Connecté au serveur : %s \n", ADRESSE_IP_INTERFACE_SERVEUR);


    // Fermeture de la connexion au serveur
    close(sock);
    printf("Déconnecté du serveur : %s \n", ADRESSE_IP_INTERFACE_SERVEUR);

    return 0;
}
