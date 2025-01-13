/*
* Programme : Main.C
 * *****************************************************
 * Description : Ce programme présente un menu interactif permettant à l'utilisateur de choisir entre
 * exécuter un client ou un serveur. Le client génère des nombres aléatoires et les envoie à un serveur,
 * tandis que le serveur génère également des nombres aléatoires, attend des connexions de clients,
 * synchronise les données reçues, effectue des analyses et arrête le programme.
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
 *********************************************************************
 * ATTENTION :
 * Il faut exécuter le main dans la racine du projet pour faire fonctionner correctement le programme.
 * Sinon, il faudra changer les chemins des programmes ou bien compiler et exécuter manuellement chaque programme.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

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

// Fonction pour valider une adresse IP
int valider_ip(const char *ip) {
    regex_t regex;
    int reti;

    // Expression régulière pour une adresse IP valide
    reti = regcomp(&regex, "^([0-9]{1,3}\\.){3}[0-9]{1,3}$", REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "Erreur lors de la compilation de l'expression régulière\n");
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

// Fonction pour valider un port
int valider_port(int port) {
    return (port > 0 && port <= 65535);
}

int main() {
    int ret = system("clear");
    if (ret != 0) {
        perror("Erreur lors de l'exécution de la commande system");
        return 1;
    }

    int choix;
    char adresse_ip[16];
    int port;
    int nbrClientPrevus;
    char commande[256];

    afficherFichier("text/main/text_bienvenue.txt");
    printf("Veuillez choisir le programme que vous voulez lancer : \n");
    printf("Attention : Faudras compiler manuellement le programme main pour lancer les programmes \n sinon faudras changer les chemins d'execetion des programmes client & serveur dans main.c \n");

    printf("1. Client\n");
    printf("   - Le programme client permet de générer des nombres aléatoires et de les envoyer à un serveur.\n");
    printf("2. Serveur\n");
    printf("   - Le programme serveur génère aussi des nombres aléatoires mais attend des clients pour synchroniser les données et faire des analyses.\n");
    printf("Entrez le numéro correspondant à votre choix : ");

    if (scanf("%d", &choix) != 1) {
        perror("Erreur de saisie lors du choix du programme");
        return 1;
    }

    if (choix == 1) {
        printf("Vous avez choisi le programme Client.\n");
        printf("Veuillez entrer l'adresse IP du serveur auquel le client va se connecter :\n");

        if (scanf("%s", adresse_ip) != 1) {
            perror("Erreur de saisie lors de l'adresse IP du serveur");
            return 1;
        }

        while (!valider_ip(adresse_ip)) {
            printf("Adresse IP invalide. Veuillez entrer une adresse IP valide :\n");
            if (scanf("%s", adresse_ip) != 1) {
                perror("Erreur de saisie lors de l'adresse IP du serveur");
                return 1;
            }
        }

        printf("Veuillez entrer le port du serveur :\n");

        if (scanf("%d", &port) != 1) {
            perror("Erreur de saisie lors du port du serveur");
            return 1;
        }

        while (!valider_port(port)) {
            printf("Port invalide. Veuillez entrer un port valide (1-65535) :\n");
            if (scanf("%d", &port) != 1) {
                perror("Erreur de saisie lors du port du serveur");
                return 1;
            }
        }

        snprintf(commande, sizeof(commande), "gcc -O3 client.c -o client && ./client %s:%d", adresse_ip, port);
        ret = system(commande);
        if (ret != 0) {
            perror("Erreur lors de l'exécution de la commande system pour le client");
            return 1;
        }

    } else if (choix == 2) {
        printf("Vous avez choisi le programme Serveur.\n");
        printf("Veuillez entrer l'adresse IP sur laquelle le serveur doit écouter :\n");

        if (scanf("%s", adresse_ip) != 1) {
            perror("Erreur de saisie lors de l'adresse IP du serveur");
            return 1;
        }

        while (!valider_ip(adresse_ip)) {
            printf("Adresse IP invalide. Veuillez entrer une adresse IP valide :\n");
            if (scanf("%s", adresse_ip) != 1) {
                perror("Erreur de saisie lors de l'adresse IP du serveur");
                return 1;
            }
        }

        printf("Veuillez entrer le port sur lequel le serveur doit écouter :\n");

        if (scanf("%d", &port) != 1) {
            perror("Erreur de saisie lors du port du serveur");
            return 1;
        }

        while (!valider_port(port)) {
            printf("Port invalide. Veuillez entrer un port valide (1-65535) :\n");
            if (scanf("%d", &port) != 1) {
                perror("Erreur de saisie lors du port du serveur");
                return 1;
            }
        }

        printf("Veuillez entrer le nombre de clients à attendre (max 100) :\n");

        if (scanf("%d", &nbrClientPrevus) != 1) {
            perror("Erreur de saisie lors du nombre de clients");
            return 1;
        }

        while (nbrClientPrevus < 1 || nbrClientPrevus > 100) {
            printf("Nombre de clients invalide. Veuillez entrer un nombre entre 1 et 100 :\n");
            if (scanf("%d", &nbrClientPrevus) != 1) {
                perror("Erreur de saisie lors du nombre de clients");
                return 1;
            }
        }

        snprintf(commande, sizeof(commande), "gcc -O3 serveur.c -o serveur && ./serveur %s:%d %d", adresse_ip, port, nbrClientPrevus);
        ret = system(commande);
        if (ret != 0) {
            perror("Erreur lors de l'exécution de la commande system pour le serveur");
            return 1;
        }

    } else {
        printf("Choix invalide, Veuillez relancer le programme et choisir un numéro valide\n");
        return 1;
    }

    return 0;
}
