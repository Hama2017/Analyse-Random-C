/*
* Programme : Serveur.C
 * *****************************************************
 * Description : Ce programme génère et synchronise les occurrences des nombres aléatoires dans un tableau en IPC (mémoire partagée), écoute les connexions des clients,
 * reçoit leurs tableaux via un socket (Communication TCP/IP) et les synchronise avec son propre tableau.
 * Il effectue ensuite des calculs statistiques et export de donnees en csv.
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

#include <math.h>
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
#include <regex.h>

#define MAX_CLIENTS 100  // Le nombre maximum de clients dans la file d'attente du serveur
#define TAILLE_TABLEAU (1<<28)  // Taille du tableau des occurences (2^28 éléments)
#define NBR_PROCESSUS 6         // Nombre de processus
#define NBR_CYCLES 10             // Nombre de cycles
#define NBR_RANDOMS (10000000000LL) // Nombres aléatoires par cycle (10 milliard)
#define SHM_KEY 0x787       // Clé pour la mémoire partagée

// Variable pour stocker le message formater avec sprintf pour inclure des variables dans les logs afin d'avoir un log riche
char message_log[600];

// Décrémenter sémaphore (verrouiller)
struct sembuf bloquer_semaphore = {0, -1, 0};
// Incrémenter sémaphore (déverrouiller)nbr_total_rand_generer
struct sembuf debloquer_semaphore = {0, 1, 0};

// Le Ration entre le Max Occurence et Min Occurence
double ratio=0;
// L'occurence la plus grande
long long max_occurence=0;
// L'occurence la plus petite
long long min_occurence=0;

/*  Le nombre de clients a traiter prevus par le serveur
*  Pour faire l'etude ont avais utiliser nous 19 clients pour nbrClientPrevus car ont a utiliser 10 ordinateurs de la salle wifi  en memme temps
*  Avec les parametres suivant le client et le serveur genere chacun 60 milliard
*   10 + 1 = 11 * 60 milliard = 660 milliards
*  pour combler le reste ont a relancer le programme client sur 9 ordinateurs
*  9 * 60 milliard = 5400 milliard
*  5400 milliard + 660 milliard = 12000 milliard
*  En tout avec les 19 clients ont genre 12000 milliard de nombre aleatoire rapidement
int nbrClientPrevus=1;
*/

// Le nombre de clients traiter  par le serveur
int nbrClientTotalTraiter=0;

//nbr_total_rand_generer = 600 milliard pour notre cas
// Le nombre total de nombres aléatoires à générer par tout les clients a traiter + le serveur
// 1200 milliard si le programme client et serveur genere chacun 600 milliard  (1 client + le serveur)
long long nbr_total_rand_generer  = 1200000000000LL;

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

/*
 * Fonction : valider_ip
 * ---------------------
 * Cette fonction valide une adresse IP en utilisant une expression régulière.
 *
 * Paramètres :
 * - ip : pointeur vers une chaîne de caractères représentant l'adresse IP à valider.
 *
 * Retourne :
 * - 1 si l'adresse IP est valide.
 * - 0 sinon.
 */
int valider_ip(const char *ip) {
    regex_t regex;
    int reti;

    // Expression régulière pour une adresse IP valide
    reti = regcomp(&regex, "^([0-9]{1,3}\\.){3}[0-9]{1,3}$", REG_EXTENDED);
    if (reti) {
        log_printf("Erreur ~ Erreur lors de la compilation de l'expression régulière\n");
        return 0;
    }

    // Exécution de l'expression régulière
    reti = regexec(&regex, ip, 0, NULL, 0);
    regfree(&regex);
    if (!reti) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Fonction : valider_port
 * -----------------------
 * Cette fonction valide un numéro de port.
 *
 * Paramètres :
 * - port : entier représentant le numéro de port à valider.
 *
 * Retourne :
 * - 1 si le numéro de port est valide (entre 1 et 65535).
 * - 0 sinon.
 */
int valider_port(int port) {
    return (port > 0 && port <= 65535);
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
    // Allocation dynamique
    struct timeval start, end;
    int *tableau_local = malloc(TAILLE_TABLEAU * sizeof(int));
    if (tableau_local == NULL) {
        perror("Erreur d'allocation mémoire pour le tableau local");
        log_printf( "Erreur ~ Erreur d'allocation mémoire pour le tableau local du processus %d \n", process_id);
        exit(1);
    }
    memset(tableau_local, 0, TAILLE_TABLEAU * sizeof(int));
    gettimeofday(&start, NULL);
    for (int j = 0; j < NBR_CYCLES; ++j) {
        log_printf( "Info ~ Processus %d : Synchronisation cycle %d ...\n", process_id, j + 1);
        for (long i = 0; i < NBR_RANDOMS; i++) {
            int rand_num = random() % TAILLE_TABLEAU;
            tableau_local[rand_num]++;
        }
        // Verrouillage du semaphore
        semop(sem_id, &bloquer_semaphore, 1);
        for (int i = 0; i < TAILLE_TABLEAU; i++) {
            tableau_IPC[i] += tableau_local[i];
        }
        // Déverrouillage du semaphore
        semop(sem_id, &debloquer_semaphore, 1);
        log_printf("Succes ~ Processus %d : Synchronisation cycle %d terminée \n", process_id, j + 1);
        memset(tableau_local, 0, TAILLE_TABLEAU * sizeof(int));
    }
    gettimeofday(&end, NULL);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    log_printf("Succes ~ Processus %d : Génération et synchronisation terminées en %.2f secondes \n", process_id, elapsed_time);
    // Libération de la mémoire
    free(tableau_local);
}

/*
 * Fonction : generer_csv_index_occurence
 * --------------------------------------
 * Cette fonction crée un fichier CSV contenant les données d'un tableau,
 * où chaque ligne représente un index et l'occurrence correspondante.
 * Le fichier est nommé "donnees_index_occurence.csv".
 *
 * Paramètres :
 * - tableau : pointeur vers le tableau d'entiers à enregistrer.
 * - taille : taille du tableau, représentant le nombre total d'occurrences.
 *
 * Fonctionnement :
 * - Un fichier CSV est créé ou écrasé s'il existe déjà.
 * - La première ligne du fichier contient l'entête "Index,Occurence".
 * - Chaque ligne suivante contient un index et sa valeur associée dans le tableau.
 */
void generer_csv_index_occurence(int *tableau, int taille, char *suffixe) {
    char nom_fichier[256];
    // Créer un nom de fichier
    snprintf(nom_fichier, sizeof(nom_fichier), "CSV/donnees_index_occurence_%s.csv", suffixe);
    // Ouvrir le fichier en mode écriture
    FILE *file = fopen(nom_fichier, "w");
    if (file == NULL) {
        perror("Erreur lors de la création du fichier");
        log_printf("Erreur ~ Erreur lors de l'ouverture du fichier %s \n", nom_fichier);
        exit(1);
    }
    // Enregistrer les données dans le fichier CSV
    fprintf(file, "Index,Occurence\n");  // Entête du CSV
    for ( int i = 0; i < taille; i++) {
        fprintf(file, "%d,%d\n", i, tableau[i]);  // Index et Occurrence
    }
    // Fermer le fichier après l'enregistrement
    fclose(file);
    snprintf(message_log, sizeof(message_log), "Succes ~ Les donnees generer_csv_index_occurence ont ete enregistrees dans le fichier %s\n", nom_fichier);

    // Passer le message formaté à log_printf
    log_printf(message_log);
}

/*
 * Fonction : generer_csv_minO_maxO_ratio
 * --------------------------------------
 * Cette fonction génère un fichier CSV contenant des statistiques calculées
 * sur les occurrences, incluant la valeur maximale, la valeur minimale,
 * et le ratio calculé entre ces deux valeurs.
 * Le fichier généré est nommé "donnees_ration_max_min.csv".
 *
 * Paramètres :
 * - max_occurence : valeur maximale trouvée dans les données (type long long).
 * - min_occurence : valeur minimale trouvée dans les données (type long long).
 * - ratio : ratio calculé entre les occurrences maximales et minimales (type double).
 *
 * Fonctionnement :
 * - Un fichier CSV est créé ou écrasé s'il existe déjà.
 * - La première ligne du fichier contient l'entête "Moyenne,Max,Min".
 * - La deuxième ligne contient le ratio, la valeur maximale et la valeur minimale.
 * - Le fichier est fermé après l'écriture des données.
 */
void generer_csv_minO_maxO_ratio(long long max_occurence, long long min_occurence, double ratio) {
    char nom_fichier[256];
    // Générer un nom de fichier
    snprintf(nom_fichier, sizeof(nom_fichier), "CSV/donnees_ratio_max_min.csv");
    // Ouvrir le fichier en mode écriture
    FILE *file = fopen(nom_fichier, "w");
    if (file == NULL) {
        perror("Erreur lors de la création du fichier CSV");
        log_printf("Erreur ~ Erreur lors de la création du fichier CSV %s \n", nom_fichier);
        exit(1);
    }
    // Enregistrer les statistiques dans le fichier CSV
    fprintf(file, "Ratio,Max,Min\n");  // Entête du CSV
    fprintf(file, "%.2f,%lld,%lld\n", ratio, max_occurence, min_occurence);  // Valeurs calculées
    // Fermer le fichier après l'enregistrement
    fclose(file);
    snprintf(message_log, sizeof(message_log), "Succes ~ Les donnees generer_csv_minO_maxO_ratio ont ete enregistrees dans le fichier %s\n", nom_fichier);

    // Passer le message formaté à log_printf
    log_printf(message_log);
    }

/*
 * Fonction : trouver_max_occurence
 * --------------------------------
 * Cette fonction recherche la valeur maximale dans un tableau d'entiers.
 *
 * Paramètres :
 * - tableau : pointeur vers le tableau d'entiers dans lequel chercher la valeur maximale.
 * - taille : nombre d'éléments dans le tableau.
 *
 * Retourne :
 * - La valeur maximale trouvée dans le tableau sous forme de long long.
 */
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
    // Retourne la valeur maximale trouvée
    return valeur_max;
}

/*
 * Fonction : trouver_min_occurence
 * --------------------------------
 * Cette fonction recherche la valeur minimale dans un tableau d'entiers.
 *
 * Paramètres :
 * - tableau : pointeur vers le tableau d'entiers dans lequel chercher la valeur minimale.
 * - taille : nombre d'éléments dans le tableau.
 *
 * Retourne :
 * - La valeur minimale trouvée dans le tableau sous forme de long long.
 */
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
    // Retourne la valeur minimale trouvée
    return valeur_min;
}

int main(int argc, char *argv[]) {

/*
 * PARTIE 0 - Lancement du programme Serveur
 * Il prend en entrée trois arguments :
 * 1. Le format `adresseIP:port` : Une chaîne de caractères qui spécifie l'adresse IP et le port
 * auquel le serveur va écouter.
 * 2. Le nombre de clients à attendre : Un entier qui spécifie le nombre de clients que le serveur doit attendre.
 */


// Vérification du nombre d'arguments
if (argc != 3) {
    /*
     * Si l'utilisateur n'a pas fourni suffisamment d'arguments,
     * nous affichons un message d'erreur et terminons le programme.
     *
     * Pour un bon usage, l'utilisateur doit lancer le programme avec :
     * ./programme adresseIP:port nbrClientPrevus
     */
    log_printf("Erreur ~ Le programme doit prendre exactement deux arguments : adresseIP:port nbrClientPrevus\n");
    log_printf("Info ~ Exemple : ./programme 127.0.0.1:8080 10\n");
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
    log_printf("Erreur ~ Format invalide, utilisez le format adresseIP:port\n");
    log_printf("Info ~ Exemple : ./programme 127.0.0.1:8080\n");
    return 1;
}

// Séparation de l'adresse IP et du port
*separateur = '\0'; // Remplace ':' par '\0' pour diviser la chaîne
char *adresse_ip_serveur = input; // La partie avant ':' devient l'adresse IP
char *port_serveur_str = separateur + 1; // La partie après ':' devient le port

// Validation de l'adresse IP
if (!valider_ip(adresse_ip_serveur)) {
    log_printf("Erreur ~ Adresse IP invalide. Utilisez le format adresseIP:port\n");
    log_printf("Info ~ Exemple : ./programme 127.0.0.1:8080\n");
    return 1;
}

// Conversion de la chaîne de caractères en entier pour le port
int port_serveur = atoi(port_serveur_str); // Port du serveur

// Validation du port
if (!valider_port(port_serveur)) {
    log_printf("Erreur ~ Port invalide. Utilisez un port entre 1 et 65535.\n");
    return 1;
}

// Extraction du nombre de clients à attendre
int nbrClientPrevus = atoi(argv[2]);

// Validation du nombre de clients
if (nbrClientPrevus <= 0 || nbrClientPrevus > 100) {
    log_printf("Erreur ~ Le nombre de clients doit être entre 1 et 100.\n");
    return 1;
}

    snprintf(message_log, sizeof(message_log),"Succès ~ Format valide. Adresse IP : %s, Port : %d, Nombre de clients : %d\n", adresse_ip_serveur, port_serveur, nbrClientPrevus);
    log_printf(message_log);
    log_printf(message_log);

    /*
     * PARTIE 1 - Initialisation et affichage
     * Cette partie affiche un message de bienvenue à partir d'un fichier
     * et démarre le serveur.
     */

    afficherFichier("text/serveur/text_bienvenue.txt");
    log_printf("Info ~ Lancement du programme serveur... \n");

    /*
     * PARTIE 2 - Mémoire partagée et sémaphores
     * Dans cette section, nous créons un segment de mémoire partagée,
     * un sémaphore pour la synchronisation et nous initialisons le tableau partagé.
     */

    pid_t pid;
    int shm_id, sem_id;
    int *tableau_IPC;

    log_printf("Info ~ Création d'un segment de mémoire partagée... \n");

    shm_id = shmget(SHM_KEY, TAILLE_TABLEAU * sizeof(int), IPC_CREAT | 0666);
    if (shm_id == -1) {
        log_printf("Erreur ~ Impossible de créer le segment de mémoire partagée\n");
        exit(EXIT_FAILURE);
    }
    log_printf("Succes ~ Segment de mémoire partagée créé \n");

    tableau_IPC = (int *)shmat(shm_id, NULL, 0);
    if (tableau_IPC == (void *)-1) {
        log_printf("Erreur ~ Impossible d'attacher le segment de mémoire partagée\n");
        exit(EXIT_FAILURE);
    }
    log_printf("Succes ~ Segment de mémoire partagée attaché");

    memset(tableau_IPC, 0, TAILLE_TABLEAU * sizeof(int));
    log_printf("Info ~ Mémoire partagée initialisée à zéro\n");

    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        log_printf("Erreur ~ Impossible de créer le sémaphore\n");
        exit(EXIT_FAILURE);
    }
    semctl(sem_id, 0, SETVAL, 1);
    log_printf("Succes ~ Sémaphore créé avec ID : %d et valeur initiale : 1\n", sem_id);


    /*
     * PARTIE 3 - Création des processus enfants
     * Cette partie gère la création et la synchronisation de plusieurs processus
     * enfants qui génèrent des nombres aléatoires.
     */

    log_printf("Info ~ Création des processus enfants...\n");

    for (int p = 0; p < NBR_PROCESSUS; p++) {
        pid = fork();
        if (pid == 0) {
            generer_synchroniser_randoms(p, tableau_IPC, sem_id);
            shmdt(tableau_IPC);
            exit(EXIT_SUCCESS);
        } else if (pid < 0) {
            log_printf("Erreur ~ Impossible de créer un processus enfant\n");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NBR_PROCESSUS; i++) {
        wait(NULL);
    }

    log_printf("Succes ~ Tous les processus enfants ont terminé\n");

    /*
    * PARTIE 4 - Création et configuration de la socket seveur, et client
    * Cette section initialise une socket serveur et client, associe le socket avec une adresse ip et un port
    * pour ecouter les connexions clients, fait le traitement des donnees et genere les differents statistiques.
    */

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    int sock_serveur = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_serveur < 0) {
        log_printf("Erreur ~ Impossible de créer la socket serveur\n");
        exit(EXIT_FAILURE);
    }

    log_printf("Succes ~ Socket serveur créée\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(adresse_ip_serveur);
    server_addr.sin_port = htons(port_serveur);

    if (bind(sock_serveur, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        log_printf("Erreur ~ Impossible de lier la socket au port %d\n", adresse_ip_serveur, port_serveur);
        exit(EXIT_FAILURE);
    }
    log_printf("Succes ~ Socket liée au port %d.\n", port_serveur);

    if (listen(sock_serveur, MAX_CLIENTS) < 0) {
        log_printf("Erreur ~ Impossible d'écouter sur la socket\n");
        exit(EXIT_FAILURE);
    }
    log_printf("Info ~ Serveur en attente de connexions clients...\n");

    int nbrClientTotalTraiter = 0;
    while (nbrClientTotalTraiter < nbrClientPrevus) {
        snprintf(message_log, sizeof(message_log),"Info ~ Serveur en Attente de client... sur %s:%d\n", adresse_ip_serveur, port_serveur);

        log_printf(message_log);


    // Accepter une nouvelle connexion client
    int sock_nouveau_client = accept(sock_serveur, (struct sockaddr *)&client_addr, &client_len);
    if (sock_nouveau_client < 0) {
        log_printf("Erreur ~ Impossible d'accepter une connexion client\n");
        exit(EXIT_FAILURE);
    }

    log_printf("Info ~ Conversion de l'adresse IP du client\n");

    // Convertir l'adresse IP du client en chaîne de caractères
    char adresse_ip_client[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, adresse_ip_client, INET_ADDRSTRLEN) == NULL) {
        log_printf("Erreur ~ Échec de la conversion de l'adresse IP du client\n");
        close(sock_nouveau_client);
        exit(EXIT_FAILURE);
    }
        snprintf(message_log, sizeof(message_log),"Succès ~ |%s| Connexion Client acceptée\n", adresse_ip_client);
        log_printf(message_log);


    // Allouer de la mémoire pour recevoir le tableau du client
    int *tableau_client_recu = (int *)malloc(TAILLE_TABLEAU * sizeof(int));
    if (tableau_client_recu == NULL) {
        snprintf(message_log, sizeof(message_log), "Erreur ~ |%s| Allocation mémoire échouée pour le tableau client\n", adresse_ip_client);
        log_printf(message_log);
        close(sock_nouveau_client);  // Fermer la connexion en cas d'erreur
        exit(EXIT_FAILURE);
    }

    snprintf(message_log, sizeof(message_log), "Info ~ |%s| Réception du Tableau du client\n", adresse_ip_client);
    log_printf(message_log);

    // Lire le tableau envoyé par le client
    if (read(sock_nouveau_client, tableau_client_recu, TAILLE_TABLEAU * sizeof(int)) == -1) {
        snprintf(message_log, sizeof(message_log), "Erreur ~ |%s| Échec lors de la lecture du tableau du client\n", adresse_ip_client);
        log_printf(message_log);
        perror("Erreur lors de la lecture");
        free(tableau_client_recu);  // Libérer la mémoire allouée en cas d'erreur
        close(sock_nouveau_client); // Fermer la connexion en cas d'erreur
        continue; // Passer au prochain client
    }

    snprintf(message_log, sizeof(message_log), "Succès ~ |%s| Le Tableau du client a été reçu par le serveur\n", adresse_ip_client);
    log_printf(message_log);

    snprintf(message_log, sizeof(message_log), "Info ~ |%s| Synchronisation du tableau du client avec le tableau du serveur...\n", adresse_ip_client);
    log_printf(message_log);

    // Synchroniser le tableau reçu avec le tableau du serveur
    for (int i = 0; i < TAILLE_TABLEAU; i++) {
        tableau_IPC[i] += tableau_client_recu[i];
    }

    snprintf(message_log, sizeof(message_log), "Succès ~ |%s| Le Tableau du client a été synchronisé avec le tableau du serveur\n", adresse_ip_client);
    log_printf(message_log);

    // Fermer la connexion avec le client
    close(sock_nouveau_client);
    snprintf(message_log, sizeof(message_log), "Succès ~ |%s| Le client a été déconnecté du serveur\n", adresse_ip_client);
    log_printf(message_log);

    // Libérer la mémoire allouée pour le tableau du client
    free(tableau_client_recu);

    nbrClientTotalTraiter++;
    snprintf(message_log, sizeof(message_log), "Info ~ %d/%d clients traités - il reste %d client(s) à traiter\n", nbrClientTotalTraiter, nbrClientPrevus, nbrClientPrevus - nbrClientTotalTraiter);
    log_printf(message_log);


}

    /*
     * PARTIE 5 - Génération des fichiers CSV pour les statistiques
     * Ici, nous générons les fichiers CSV pour les statistiques globales et les index/occurrences.
     */

    // Calculer les statistiques (moyenne, min, max)

    max_occurence = trouver_max_occurence(tableau_IPC, TAILLE_TABLEAU);
    min_occurence = trouver_min_occurence(tableau_IPC, TAILLE_TABLEAU);
    ratio = (double)(max_occurence - min_occurence) / ((double)nbr_total_rand_generer);

    log_printf("Info ~ Calcul des statistiques...\n");

    log_printf("Info ~ Generation du fichier CSV pour les statistiques...\n");

    // Generer le fichier CSV  contenant le ratio, le minOccurence et le maxOccurence
    generer_csv_minO_maxO_ratio(min_occurence,max_occurence,ratio);

    // Generer le fichier CSV pour les Index et Occurrences
    generer_csv_index_occurence(tableau_IPC, TAILLE_TABLEAU, "complet");


    /*
     * PARTIE 6 - Nettoyage des ressources
     * Ici, nous libérons les ressources allouées :
     * mémoire partagée, sémaphore, et fermeture de la socket.
     */

    log_printf("Info ~ Liberation de la memoire ....\n");

    shmdt(tableau_IPC);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    close(sock_serveur);
    log_printf("Succes ~ Programme serveur a terminé avec succès \n");

    // Log des statistiques
    snprintf(message_log, sizeof(message_log), "Succès ~ MaxOccurence: %lld\n", max_occurence);
    log_printf(message_log);
    snprintf(message_log, sizeof(message_log), "Succès ~ MinOccurence: %lld\n", min_occurence);
    log_printf(message_log);
    snprintf(message_log, sizeof(message_log), "Succès ~ Ratio: %.2f\n", ratio);
    log_printf(message_log);

    printf("\n\n");
    afficherFichier("text/serveur/text_fin.txt");
    printf("\n\n");

    // Fin du programme serveur

    return 0;
}
