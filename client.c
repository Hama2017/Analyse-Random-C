/*
 * Programme : Client.C
 * *****************************************************
 * Description : Ce programme génère et synchronise les occurrences des nombres aléatoires dans un tableau en IPC (mémoire partagée)
 * et envoie le tableau de l'IPC à un serveur pour traitement via un socket.
 ********************************************************************
 * Le code a été réalisé par BA Hamadou et BA Salimatouh Maliah.
 * 15 Décembre 2024.
 * Le projet porte le nom "Etude Fonction rand".
 * Lien du dépôt GitHub : https://github.com/Hama2017/Analyse-Random-C
 *********************************************************************
 * Bibliothèques utilisées :
 * - <stdarg.h> : Permet de gérer des arguments variables pour la fonction de log.
 * - <stdio.h> : Fournit des fonctions de base d'entrée/sortie, comme printf, pour afficher et enregistrer des messages.
 * - <stdlib.h> : Contient des fonctions utilitaires comme malloc et random.
 * - <unistd.h> : Permet l'utilisation de fonctionnalités POSIX telles que sleep et fork.
 * - <arpa/inet.h> : Utilisé pour la gestion des adresses IP et des sockets.
 * - <sys/socket.h> : Fournit des fonctions pour la création et la gestion de sockets.
 * - <sys/ipc.h>, <sys/shm.h>, <sys/sem.h> : Gèrent la mémoire partagée et les sémaphores pour la synchronisation.
 * - <sys/time.h> : Permet de mesurer le temps écoulé pendant l'exécution du programme.
 * - <string.h> : Fournit des fonctions utilitaires pour la manipulation de chaînes de caractères.
 */

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
#include <time.h>
#include <string.h>

#define TAILLE_TABLEAU (1<<28)  // Taille du tableau des occurences (2^28 éléments)
#define NBR_PROCESSUS 6         // Nombre de processus
#define NBR_CYCLES 10             // Nombre de cycles
#define NBR_RANDOMS (10000000000LL) // Nombres aléatoires par cycle (10 milliard)
#define SHM_KEY 0x874         // Clé pour la mémoire partagée

// Variable pour stocker le message formater avec sprintf pour inclure des variables dans les logs afin d'avoir un log riche
char message_log[600];

struct sembuf bloquer_semaphore = {0, -1, 0}; // Décrémenter sémaphore (verrouiller)
struct sembuf debloquer_semaphore = {0, 1, 0}; // Incrémenter sémaphore (déverrouiller)

// Fonction pour obtenir l'heure actuelle sous forme de chaîne formatée
/*
 * Cette fonction récupère l'heure actuelle du système et la formate
 * sous la forme "[HH:MM:SS]".
 *
 * Paramètres :
 * - buffer : Pointeur vers une chaîne où l'heure formatée sera stockée (type char*).
 * - taille : Taille maximale du buffer (type size_t).
 */
void obtenir_temps_actuel(char *buffer, size_t taille) {
    // Temps brut du système
    time_t rawtime;
    // Structure pour l'heure locale
    struct tm *timeinfo;
    // Récupérer le temps actuel
    time(&rawtime);
    // Convertir en heure locale
    timeinfo = localtime(&rawtime);
    // Formater l'heure
    strftime(buffer, taille, "[%H:%M:%S]", timeinfo);
}

// Fonction de log qui écrit à la fois dans la console et dans un fichier
/*
 * Cette fonction permet d'enregistrer des logs à la fois dans la console
 * et dans un fichier texte, avec horodatage.
 *
 * Paramètres :
 * - format : Chaîne de formatage (type const char*).
 * - ... : Arguments variables pour le formatage.
 *
 * Fonctionnement :
 * - Ajout d'un horodatage à chaque message.
 * - Affichage dans la console avec printf().
 * - Écriture dans un fichier log situé dans "log/serveur/log.txt".
 */
void log_printf(const char *format, ...) {
    // Buffer pour l'heure formatée [HH:MM:SS]
    char temps_actuel[11];
    obtenir_temps_actuel(temps_actuel, sizeof(temps_actuel));
    // Ouvrir en mode ajout
    FILE *logfile = fopen("log/serveur/log.txt", "a");
    if (logfile == NULL) {
        perror("Erreur lors de l'ouverture du fichier de log");
        exit(1);
    }
    // Initialisation des arguments variables
    va_list args;
    va_start(args, format);
    // Affichage dans la console
    printf("%s ", temps_actuel);
    vprintf(format, args);
    // Écriture dans le fichier
    fprintf(logfile, "%s ", temps_actuel);
    vfprintf(logfile, format, args);
    // Terminer les arguments variables
    va_end(args);
    // Fermer le fichier
    fclose(logfile);
}

// Fonction pour afficher le contenu d'un fichier texte
/*
 * Cette fonction ouvre le fichier spécifié, lit son contenu ligne par ligne,
 * et l'affiche à l'écran.
 *
 * Paramètres :
 * - nom_fichier : Chemin du fichier à lire (type const char*).
 */
void afficherFichier(const char *nom_fichier) {
    FILE *file = fopen(nom_fichier, "r");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        log_printf("Erreur ~ Erreur lors de l'ouverture du fichier %s \n", nom_fichier);
        exit(1);
    }
    // Buffer pour lire les lignes du fichier
    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        // Affichage ligne par ligne
        printf("%s", line);
    }
    // Fermeture du fichier
    fclose(file);
}

// Fonction pour générer et synchroniser les nombres aléatoires pour les processus
/*
 * Cette fonction génère des nombres aléatoires, met à jour un tableau partagé
 * synchronisé avec un sémaphore, et mesure le temps total de l'opération.
 *
 * Paramètres :
 * - process_id : Identifiant du processus (type int).
 * - tableau_IPC : Tableau partagé à mettre à jour (type int*).
 * - sem_id : Identifiant du sémaphore pour la synchronisation (type int).
 *
 * Fonctionnement :
 * - Un tableau local est alloué dynamiquement et initialisé.
 * - À chaque cycle, des nombres aléatoires sont générés et stockés
 *   dans le tableau local.
 * - Le tableau global est mis à jour en utilisant un verrou global
 *   (granularité grossière).
 * - Le temps total écoulé est mesuré et enregistré.
 * - La mémoire allouée pour le tableau local est libérée à la fin.
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

int main(int argc, char *argv[]) {

    /*
 * PARTIE 0 - Lancement du programme Client
 * Il prend en entrée deux arguments :
 * 1. Le format `adresseIP:port` : Une chaîne de caractères qui spécifie l'adresse IP et le port du serveur
 * auquel le tableau IPC seras transmit.
 */

    // Vérification du nombre d'arguments
    if (argc < 2) {
        /*
         * Si l'utilisateur n'a pas fourni suffisamment d'arguments,
         * nous affichons un message d'erreur et terminons le programme
         *
         * Pour un bon usage, l'utilisateur doit lancer le programme avec :
         * ./programme adresseIP:port
         */
        snprintf(message_log, sizeof(message_log),
                 "Erreur ~ Le programme doit prendre exactement un argument : adresseIP:port\n");
        log_printf(message_log);

        snprintf(message_log, sizeof(message_log), "Info ~ Exemple : ./programme 127.0.0.1:8080\n");
        log_printf(message_log);

        return 1;
    }

    // Extraction de l'argument (format attendu : adresseIP:port)
    char *input = argv[1]; // Premier argument après le nom du programme
    char *separateur = strchr(input, ':'); // Recherche du caractère ':'

    if (separateur == NULL) {
        /*
         * Si le caractère ':' n'est pas trouvé dans l'argument,
         * cela signifie que le format fourni par l'utilisateur est incorrect.
         * Nous affichons un message d'erreur et terminons le programme.
         */

        snprintf(message_log, sizeof(message_log), "Erreur ~ Format invalide, utilisez le format adresseIP:port \n");
        log_printf(message_log);

        snprintf(message_log, sizeof(message_log), "Info ~ Exemple : ./programme 127.0.0.1:8080\n");
        log_printf(message_log);

        return 1;
    }

    // Séparation de l'adresse IP et du port
    *separateur = '\0'; // Remplace ':' par '\0' pour diviser la chaîne
    char *adresse_ip_serveur = input; // La partie avant ':' devient l'adresse IP
    char *port_serveur_str = separateur + 1;
    // La partie après ':' devient (le port) car c'est en string faudras le convertir

    // Conversion de la chaîne de caractères en entier pour le port
    // La fonction atoi() transforme la chaîne `port_serveur_str` en un entier (int) qui représente le numéro de port
    int port_serveur = atoi(port_serveur_str); // Port du serveur

    // Vérification des valeurs extraites
    if (strlen(adresse_ip_serveur) == 0 || strlen(port_serveur_str) == 0 || port_serveur <= 0 || port_serveur > 65535) {
        /*
         * Si l'adresse IP ou le port sont vides, cela indique que l'utilisateur
         * a fourni un format incomplet, par exemple ":8080" ou "127.0.0.1:".
         * Nous affichons un message d'erreur et terminons le programme.
         */

        snprintf(message_log, sizeof(message_log),
                 "Erreur ~ L'adresse IP ou le port est invalide. Assurez-vous d'utiliser le format adresseIP:port.\n");
        log_printf(message_log);

        snprintf(message_log, sizeof(message_log), "Info ~ Exemple : ./programme 127.0.0.1:8080\n");
        log_printf(message_log);
        return 1;
    }

    snprintf(message_log, sizeof(message_log), "Succes ~ Format valide. Adresse IP : %s, Port : %s\n",
             adresse_ip_serveur, port_serveur_str);
    log_printf(message_log);


    /*
     * PARTIE 1 - Initialisation et affichage
     * Cette partie affiche un message de bienvenue à partir d'un fichier.
     * et démarre le client.
     */

    afficherFichier("text/client/text_bienvenue.txt");

    // Affichage d'un message au début du lancement du programme client
    log_printf("Info ~ Lancement du programme client... \n");

    /*
     * PARTIE 2 - Mémoire partagée et sémaphores
     * Dans cette section, nous créons un segment de mémoire partagée,
     * un sémaphore pour la synchronisation et nous initialisons le tableau partagé.
     */

    pid_t pid;
    int shm_id, sem_id;
    int *tableau_IPC;

    log_printf("Info ~ Création d'un segment de mémoire partagée... \n");

    // Création d'un segment de mémoire partagée
    shm_id = shmget(SHM_KEY, TAILLE_TABLEAU * sizeof(int), IPC_CREAT | 0666);
    if (shm_id == -1) {
        snprintf(message_log, sizeof(message_log),
                 "Erreur ~ Impossible de créer un segment de mémoire partagée (shmget)");
        log_printf(message_log);
        perror("Erreur shmget");
        exit(1);
    }

    log_printf("Succes ~ Création d'un segment de mémoire partagée \n");

    snprintf(message_log, sizeof(message_log),
             "Info ~ Attachement du segment de mémoire partagée au processus courant %d ... \n", getpid());
    log_printf(message_log);

    // Attachement du segment de mémoire partagée au processus courant
    tableau_IPC = (int *) shmat(shm_id, NULL, 0);
    if (tableau_IPC == (void *) -1) {
        snprintf(message_log, sizeof(message_log),
                 "Erreur ~ Échec de l'attachement du segment de mémoire partagée au processus courant %d", getpid());
        log_printf(message_log);
        perror("Erreur shmat");
        exit(1);
    }

    snprintf(message_log, sizeof(message_log),
             "Succes ~ Attachement du segment de mémoire partagée au processus courant %d \n", getpid());
    log_printf(message_log);

    log_printf("Info ~ Initialisation du tableau partagé avec des zéros... \n");

    // Initialisation du tableau partagé avec des zéros
    memset(tableau_IPC, 0, TAILLE_TABLEAU * sizeof(int));

    log_printf("Succes ~ Initialisation du tableau partagé avec des zéros \n");

    // Création d'un sémaphore pour la synchronisation des processus
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        snprintf(message_log, sizeof(message_log), "Erreur ~ Impossible de créer un sémaphore (semget)");
        log_printf(message_log);
        perror("Erreur semget");
        exit(1);
    }

    // Initialisation du sémaphore à 1 (verrou débloqué)
    semctl(sem_id, 0, SETVAL, 1);


    /*
     * PARTIE 3 - Création des processus enfants
     * Cette partie gère la création et la synchronisation de plusieurs processus
     * enfants qui génèrent des nombres aléatoires.
     */


    log_printf("Info ~ Creations des proccesus enfant en cours...\n");


    for (int p = 0; p < NBR_PROCESSUS; p++) {
        pid = fork();
        if (pid == 0) {
            snprintf(message_log, sizeof(message_log), "Info ~ Processus %d Genere des nombre aleatoires...\n", p);
            log_printf(message_log);

            // Processus enfant
            srand(getpid()); // Initialisation d'une graine pour les nombres aléatoires

            generer_synchroniser_randoms(p, tableau_IPC, sem_id);
            //shmdt(tableau_IPC); // Détachement de la mémoire partagée

            snprintf(message_log, sizeof(message_log), "Succes ~ Processus %d Genere des nombre aleatoires\n", p);
            log_printf(message_log);
            shmdt(tableau_IPC); // Détachement de la mémoire partagée

            exit(0); // Terminaison du processus enfant
        }
    }


    // Attente de la fin de tous les processus enfants
    for (int p = 0; p < NBR_PROCESSUS; p++) {
        wait(NULL);
    }

    log_printf("Succes ~ Génération de nombres aléatoires terminée par tout les procces \n");

    /*
     * PARTIE 4 - Création et configuration de la socket
     * Cette section initialise une socket, se connecte au serveur
     * et envoie le tableau partagé IPC.
     */

    log_printf("Info ~ Preparation envoie du tableau IPC au serveur... \n");


    int sock = 0; // Déclaration de la variable pour stocker le descripteur de la  socket
    struct sockaddr_in serv_addr; // Déclaration de la structure pour les informations de l'adresse du serveur

    log_printf("Info ~ Création du socket... \n");


    // Créer une nouvelle socket de communication réseau
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        snprintf(message_log, sizeof(message_log), "Erreur ~ Impossible de créer une socket");
        log_printf(message_log);
        perror("Erreur de création de la socket");
        return -1;
    }

    log_printf("Succes ~ Création du socket... \n");

    log_printf("Info ~ Verification validite adresse ip du serveur... \n");


    // Initialiser les informations de l'adresse du serveur
    serv_addr.sin_family = AF_INET; // Spécifier le domaine d'adresse IPv4
    serv_addr.sin_port = htons(port_serveur); // Convertir la valeur du port à l'ordre réseau (BigEndian)

    // Conversion de l'adresse IP en format binaire
    if (inet_pton(AF_INET, adresse_ip_serveur, &serv_addr.sin_addr) <= 0) {
        snprintf(message_log, sizeof(message_log), "Erreur ~ Adresse du serveur invalide");
        log_printf(message_log);
        perror("Adresse du serveur invalide");
        return -1;
    }

    log_printf("Succes ~ Verification validite adresse ip du serveur \n");


    snprintf(message_log, sizeof(message_log), "Info ~ Connexion au serveur : %s ...\n", adresse_ip_serveur);
    log_printf(message_log);


    // Connexion au serveur
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        snprintf(message_log, sizeof(message_log), "Erreur ~ Échec de la connexion au serveur");
        log_printf(message_log);
        perror("Échec de la connexion au serveur");
        return -1;
    }

    snprintf(message_log, sizeof(message_log), "Succes ~ Connexion au serveur : %s \n", adresse_ip_serveur);
    log_printf(message_log);

    log_printf("Info ~ Envoi du tableau IPC au serveur... \n");


    // Envoi du tableau partagé IPC au serveur

    if (send(sock, tableau_IPC, TAILLE_TABLEAU * sizeof(int), 0) == -1) {
        snprintf(message_log, sizeof(message_log), "Erreur ~ Échec lors de l'envoi du tableau IPC");
        log_printf(message_log);
        perror("Erreur lors de l'envoi du tableau");
    }

    snprintf(message_log, sizeof(message_log), "Succes ~ Tableau IPC envoyées avec succès au serveur %s \n",adresse_ip_serveur);
    log_printf(message_log);


    /*
     * PARTIE 5 - Nettoyage des ressources
     * Ici, nous libérons les ressources allouées :
     * mémoire partagée, sémaphore, et fermeture de la socket.
     */

    log_printf("Info ~ Nettoyage des ressources... \n");

    shmdt(tableau_IPC); // Détachement de la mémoire partagée
    shmctl(shm_id, IPC_RMID, NULL); // Suppression de la mémoire partagée
    semctl(sem_id, 0, IPC_RMID); // Suppression du sémaphore

    log_printf("Succes ~ Nettoyage des ressources... \n");

    log_printf("Info ~ Deconnexion au serveur... \n");

    // Fermeture de la connexion au serveur
    close(sock);

    snprintf(message_log, sizeof(message_log), "Succes ~ Déconnecté du serveur : %s \n", adresse_ip_serveur);
    log_printf(message_log);

    log_printf("Succes ~ Programme Client terminer \n");

    printf("\n\n");
    afficherFichier("text/client/text_fin.txt");
    printf("\n\n");

    // Fin du programme client

    return 0;
}