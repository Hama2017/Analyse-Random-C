#include <stdarg.h>
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

#define ADRESSE_IP_INTERFACE_SERVEUR "127.0.0.1"  // Adresse IP du serveur
#define MAX_CLIENTS 100  // Le nombre maximum de clients dans la file d'attente du serveur
#define PORT 8080  // Le port sur lequel le serveur écoute
#define TAILLE_TABLEAU (1<<20)  // Taille du tableau à envoyer (2^28 éléments)
#define NBR_PROCESSUS 6         // Nombre de processus
#define NBR_CYCLES 10             // Nombre de cycles
#define NBR_RANDOMS (100) // Nombres aléatoires par cycle (10 milliard)
#define SHM_KEY 0x233          // Clé pour la mémoire partagée
#define K 1024        // Taille d'un bloc pour le pliage (pour generer le csv Index,Occurence avoir une allure de la courbe )


// Déclaration des variables globales

// Variable pour stocker le message formater avec sprintf pour inclure des variables dans les logs afin d'avoir un log riche
char message_log[600];

struct sembuf bloquer_semaphore = {0, -1, 0};   // Décrémenter sémaphore (verrouiller)
struct sembuf debloquer_semaphore = {0, 1, 0}; // Incrémenter sémaphore (déverrouiller)

double ration_max_min=0; // Le Ration entre le Max Occurence et Min Occurence
long long max_occurence=0; // L'occurence la plus grande
long long min_occurence=0; // L'occurence la plus petite

int nbrClientPrevus=5; // Le nombre de clients a traiter prevus par le serveur
int nbrClientTotalTraiter=0; // Le nombre de clients traiter  par le serveur

//nbr_total_rand_prevus_generer = 600 milliard pour notre cas
// Le nombre total de nombres aléatoires à générer par tout les clients a traiter + le serveur  et on multiplie avec *1LL pour spécifier au compilateur que c'est un long long
long long nbr_total_rand_prevus_generer  = (NBR_PROCESSUS * NBR_CYCLES * NBR_RANDOMS) * (nbrClientPrevus + 1)  * 1LL;



// FONCTIONS UTILISES


// Fonction pour afficher le contenu d'un fichier texte
/*
 * Cette fonction ouvre le fichier spécifié, lit son contenu ligne par ligne et l'affiche à l'écran.
 */
void afficherFichier(const char *nomFichier) {
    FILE *file = fopen(nomFichier, "r");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
    }

    // Lecture et affichage ligne par ligne
    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
    }

    fclose(file);
}

// Fonction pour obtenir l'heure actuelle sous forme de chaîne formatée
/*
 * Cette fonction récupère l'heure actuelle du système et la formate sous la forme "[HH:MM:SS]".
 */
void obtenir_temps_actuel(char *buffer, size_t taille) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, taille, "[%H:%M:%S]", timeinfo);
}

// Fonction de log qui écrit à la fois dans la console et dans un fichier
/*
 * Cette fonction permet d'enregistrer des logs à la fois dans la console et dans un fichier texte.
 * Elle utilise un format spécifié par l'utilisateur et ajoute un horodatage à chaque message.
 */
void log_printf(const char *format, ...) {
    char temps_actuel[11]; // Buffer pour stocker l'heure formatée [HH:MM:SS]
    obtenir_temps_actuel(temps_actuel, sizeof(temps_actuel));
    // Ouvrir le fichier en mode ajout
    FILE *logfile = fopen("log/client/log.txt", "a");
    if (logfile == NULL) {
        perror("Erreur lors de l'ouverture du fichier de log");
    }
    // Initialisation de la liste des arguments variables
    va_list args;
    va_start(args, format);
    // Affichage du message sur la console
    printf("%s ", temps_actuel);
    vprintf(format, args);
    // Écriture du message dans le fichier de log
    fprintf(logfile, "%s ", temps_actuel);
    vfprintf(logfile, format, args);
    // Terminer la liste des arguments variables
    va_end(args);
    // Fermer le fichier
    fclose(logfile);
}

// Fonction pour générer et synchroniser les nombres aléatoires pour les processus
/*
 * Cette fonction génère des nombres aléatoires et les synchronise à l'aide d'un sémaphore.
 * Elle crée un tableau local pour chaque processus, génère des nombres aléatoires,
 * et les ajoute au tableau IPC partagé, tout en respectant la synchronisation entre les processus.
 */
void generer_synchroniser_randoms(int process_id, int *tableau_IPC, int sem_id) {
    // Allocation dynamique de mémoire pour le tableau local
    int *tableau_local = malloc(TAILLE_TABLEAU * sizeof(int));

    if (tableau_local == NULL) {
        // Affichage d'un message d'erreur en cas d'échec de l'allocation mémoire
        perror("Erreur d'allocation mémoire pour le tableau local");
        snprintf(message_log, sizeof(message_log),
                 "Erreur ~ Erreur d'allocation mémoire pour le tableau local du processus %d \n", process_id);
        log_printf(message_log);

        exit(1);
    }

    // Initialisation du tableau local avec des zéros
    memset(tableau_local, 0, TAILLE_TABLEAU * sizeof(int));

    // Structures pour mesurer le temps de début et de fin
    struct timeval start, end;
    gettimeofday(&start, NULL); // Obtenir le temps actuel

    // Boucle pour chaque cycle de génération de nombres aléatoires
    for (int j = 0; j < NBR_CYCLES; ++j) {
        snprintf(message_log, sizeof(message_log), "Info ~ Processus %d : Synchronisation cycle %d ...\n", process_id,
                 j + 1);
        log_printf(message_log);

        // Génération des nombres aléatoires et mise à jour du tableau local
        for (long i = 0; i < NBR_RANDOMS; i++) {
            int rand_num = random() % TAILLE_TABLEAU;
            tableau_local[rand_num]++;
        }

        /* Utilisation d'une une granularité grossière.
           * Pour la synchronisation des processus, nous avons choisi d'utiliser une granularité grossière. Cela signifie que l'accès au tableau IPC
           * est verrouillé dans son ensemble pendant que chaque processus effectue ses mises à jour. Bien que cette approche puisse entraîner des temps d'attente
          * plus longs pour les autres processus, elle simplifie la gestion de la synchronisation en évitant les conflits au niveau des éléments individuels du tableau.
        */

        // Synchronisation avec le tableau global
        semop(sem_id, &bloquer_semaphore, 1); // Verrouillage du sémaphore
        for (int i = 0; i < TAILLE_TABLEAU; i++) {
            tableau_IPC[i] += tableau_local[i];
        }
        semop(sem_id, &debloquer_semaphore, 1); // Déverrouillage du sémaphore

        snprintf(message_log, sizeof(message_log), "Succes ~ Processus %d : Synchronisation cycle %d terminée\n",
                 process_id, j + 1);
        log_printf(message_log);

        // Réinitialisation du tableau local pour le prochain cycle
        memset(tableau_local, 0, TAILLE_TABLEAU * sizeof(int));
    }

    // Obtenir le temps actuel après la fin de tous les cycles
    gettimeofday(&end, NULL);

    // Calcul du temps écoulé en secondes
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Affichage du temps total écoulé pour la génération et la synchronisation

    snprintf(message_log, sizeof(message_log),"Succes ~ Processus %d : Génération et synchronisation terminées en %.2f secondes\n", process_id,elapsed_time);
    log_printf(message_log);

    // Libération de la mémoire allouée pour le tableau local
    free(tableau_local);
}


// Fonction pour créer un fichier CSV et y enregistrer les données du tableau IPC (Index,Occurence)
void generer_csv_index_occurence(int *tableau, int taille,char * adresse_ip_client,int indice_traitement_client) {
    char nom_fichier[256];
    // Créer un nom de fichier unique basé sur le compteur des clients traités
    snprintf(nom_fichier, sizeof(nom_fichier), "donnees_analyse_client_%s_%d.csv", adresse_ip_client, indice_traitement_client);

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

// Fonction pour générer un fichier CSV avec les statistiques
void generer_stats_csv(long long max_occurence, long long min_occurence, double moyenne, const char *adresse_ip_client, int indice_traitement_client) {
    // Créer un nom de fichier unique basé sur l'adresse IP du client et le nombre total de clients traités
    char nom_fichier[256];
    snprintf(nom_fichier, sizeof(nom_fichier), "stats_client_%s_%d.csv", adresse_ip_client, indice_traitement_client);

    // Ouvrir le fichier en mode écriture
    FILE *file = fopen(nom_fichier, "w");
    if (file == NULL) {
        perror("Erreur lors de la création du fichier CSV");
        return;
    }

    // Enregistrer les statistiques dans le fichier CSV
    fprintf(file, "Moyenne,Max,Min\n");  // Entête du CSV
    fprintf(file, "%.2f,%lld,%lld\n", moyenne, max_occurence, min_occurence);  // Valeurs calculées

    // Fermer le fichier après l'enregistrement
    fclose(file);

    printf("Les statistiques ont été enregistrées dans le fichier %s\n", nom_fichier);
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

// Fonction pour trouver le max_occurence
long long trouver_max_occurence(int *tableau, int taille) {
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

// Fonction pour trouver le min_occurence
long long trouver_min_occurence(int *tableau, int taille) {
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
    printf("DEMMARAGE DU SERVEUR.... \n");
    int sockfd, newsockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int shm_id, sem_id;
    int *tableau_IPC;

    // Création d'un segment de mémoire partagée
    shm_id = shmget(SHM_KEY, TAILLE_TABLEAU * sizeof(int), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Erreur shmget");
        exit(1);
    }

    // Attachement du segment de mémoire partagée au processus courant
    tableau_IPC = (int *)shmat(shm_id, NULL, 0);
    if (tableau_IPC == (void *)-1) {
        perror("Erreur shmat");
        exit(1);
    }

    // Initialisation du tableau partagé avec des zéros
    memset(tableau_IPC, 0, TAILLE_TABLEAU * sizeof(int));

    // Création d'un sémaphore pour la synchronisation des processus
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Erreur semget");
        exit(1);
    }

    // Initialisation du sémaphore à 1 (verrou débloqué)
    semctl(sem_id, 0, SETVAL, 1);


    printf("Calcul en cours...... \n");


    // Attention: démarrage des processus enfants avant de créer la socket
    for (int i = 0; i < NBR_PROCESSUS; i++) {
        pid_t pid = fork();
        if (pid == 0) { // Processus fils
            srand(getppid()); // Initialisation d'une graine pour les nombres aléatoires
            generer_synchroniser_randoms(i, tableau_IPC, sem_id);
            exit(0); // Exit after completing task
        }
    }

    // Attendre la fin des processus enfants
    for (int i = 0; i < NBR_PROCESSUS; i++) {
        wait(NULL);
    }

    // Création de la socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur lors de la création de la socket");
        exit(1);
    }

    // Configurer l'adresse du serveur
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ADRESSE_IP_INTERFACE_SERVEUR);
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur bind");
        exit(1);
    }

    printf("Serveur : DEMARAGE.....\n");

    // Ecoute sur le port
    if (listen(sockfd, MAX_CLIENTS) < 0) {
        perror("Erreur ecoute");
        exit(1);
    }


    // Accept connections
    while (nbrClientTotalTraiter < nbrClientPrevus) {
        printf("Serveur : Ecoute sur %s:%d  ......\n",ADRESSE_IP_INTERFACE_SERVEUR,PORT);
        client_len = sizeof(client_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (newsockfd < 0) {
            perror("Erreur accept");
            exit(1);
        }

        // Récupérer l'adresse IP du client
        char adresse_ip_client[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, adresse_ip_client, sizeof(adresse_ip_client));

        printf("Serveur : le client %s viens de se connecter  \n",adresse_ip_client);


        // Allocation de mémoire pour le tableau à recevoir
        int *tableau_client_recu = malloc(TAILLE_TABLEAU * sizeof(int));
        if (tableau_client_recu == NULL) {
            perror("Erreur d'allocation mémoire");
            close(newsockfd);
            close(sockfd);
            exit(1);
        }

        long bytes_recu = read(newsockfd, tableau_client_recu, TAILLE_TABLEAU * sizeof(int));  // Utilisation de read() pour récupérer les données

        if (bytes_recu < 0) {
            perror("Erreur read");
            free(tableau_client_recu);  // Libérer la mémoire
            exit(1);
        } else if (bytes_recu==0) {

            printf("fffofofofofofoofofof");

        }

        printf("Serveur : %ld octets reçus\n", bytes_recu);



        // Effectuer la somme du tableau client avec celui du serveur
        for (int i = 0; i < TAILLE_TABLEAU; i++) {
            tableau_IPC[i] += tableau_client_recu[i];  // Additionner les valeurs du tableau client au tableau serveur
        }

        printf("Serveur : %s -> Tableau reçu \n", adresse_ip_client);


        printf("Serveur : Confirmation Tableau recus envoyer au client -> %s   \n", adresse_ip_client);



        // Calculer les statistiques (moyenne, min, max)
        long long max_occurence = trouver_max_occurence(tableau_IPC, TAILLE_TABLEAU);
        long long min_occurence = trouver_min_occurence(tableau_IPC, TAILLE_TABLEAU);
        double moyenne = (double)(max_occurence - min_occurence) / nbr_total_rand_prevus_generer;

        // Generer le fichier CSV pour les statistique de chaque client
        generer_stats_csv(max_occurence,min_occurence,moyenne,adresse_ip_client,nbrClientTotalTraiter+1);

        // Pliage pour generer une courbe des occurences
        int nouvelle_taille;
        int *tableau_plie = plier_tableau(tableau_client_recu, TAILLE_TABLEAU, K, &nouvelle_taille);
        // Generer les fichier CSV pour les Index,Occurence a partir du tableau plier
        generer_csv_index_occurence(tableau_plie, nouvelle_taille,adresse_ip_client,nbrClientTotalTraiter+1);
        free(tableau_plie);


        // Fermer la connexion avec le client après réception
        close(newsockfd);
        nbrClientTotalTraiter++;
        printf("Serveur : deconnexion du client  %s\n", adresse_ip_client);
        printf("Serveur :  %d clients sur %d traiter il reste %d clients a traiter ....  \n", nbrClientTotalTraiter,nbrClientPrevus,nbrClientPrevus-nbrClientTotalTraiter);

    }

    printf("Serveur : Calcul les stats global .... \n");

    // Calculer les statistiques (moyenne, min, max)
    max_occurence = trouver_max_occurence(tableau_IPC, TAILLE_TABLEAU);
    min_occurence = trouver_min_occurence(tableau_IPC, TAILLE_TABLEAU);
    ration_max_min = (double)(max_occurence - min_occurence) / (nbr_total_rand_prevus_generer*(nbrClientPrevus+1));


    printf("Serveur : Generation du fichier CSV pour les stats global .... \n");

    // Generer le fichier CSV pour les statistique de chaque client
    generer_stats_csv(max_occurence,min_occurence,ration_max_min,"SERVER",0);


    printf("Serveur : Pliage du tableau pour generer le fichier CSV pour les Index,Occurences global .... \n");

    // Pliage pour generer une courbe des occurences
    int nouvelle_taille;
    int *tableau_plie = plier_tableau(tableau_IPC, TAILLE_TABLEAU, K, &nouvelle_taille);

    generer_csv_index_occurence(tableau_plie, nouvelle_taille,"SERVER",0);
    free(tableau_plie);

    printf("Serveur : Liberation de la memoire .... \n");

    // Detacher la mémoire partagée
    shmdt(tableau_IPC);

    // Detruire le segment de mémoire partagée et le sémaphore
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    printf("Serveur : DECONNECTER ");

    return 0;
}
