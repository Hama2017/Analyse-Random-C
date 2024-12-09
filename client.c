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

#define IP_ADRESSE_INTERFACE_SERVEUR "127.0.0.1"  // Adresse IP du serveur
#define PORT 8080  // Le port sur lequel le serveur écoute
#define ARRAY_SIZE (1<<10)  // Taille du tableau à envoyer (2^20 éléments)
#define CHUNK_SIZE 1024  // Taille du bloc pour l'envoi (1024 octets)
#define NUM_PROCESSES 6         // Nombre de processus
#define NBR_FOIS 10             // Nombre de cycles
#define NUM_RANDOMS (1000000000L) // Nombres aléatoires par cycle (1 milliard)
#define SHM_KEY 0x1234          // Clé pour la mémoire partagée
const int PROCESS_ENVOIE_VECTEUR =  NUM_PROCESSES - 1;
struct sembuf lock = {0, -1, 0};   // Décrémenter sémaphore (verrouiller)
struct sembuf unlock = {0, 1, 0}; // Incrémenter sémaphore (déverrouiller)
long long totalOccurence = 0;




// Fonction pour créer un fichier CSV et y enregistrer les données
void create_data_file(int *data, int data_size) {
    char filename[256];

    // Créer un nom de fichier unique basé sur le compteur des clients traités
    snprintf(filename, sizeof(filename), "donnees_analyse.csv");


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



// Fonction pour générer les nombres aléatoires et synchroniser
void generate_and_sync(int process_id, int *shared_table, int sem_id) {
    int *local_table = malloc(ARRAY_SIZE * sizeof(int));
    if (local_table == NULL) {
        perror("Erreur d'allocation mémoire pour le tableau local");
        exit(1);
    }

    memset(local_table, 0, ARRAY_SIZE * sizeof(int));

    struct timeval start, end;
    gettimeofday(&start, NULL);

    for (int j = 0; j < NBR_FOIS; ++j) {

        for (long i = 0; i < NUM_RANDOMS; i++) {
            int rand_num = random() % ARRAY_SIZE;
            local_table[rand_num]++;
        }

        // Synchronisation avec le tableau global
        semop(sem_id, &lock, 1); // Verrouillage
        for (int i = 0; i < ARRAY_SIZE; i++) {
            shared_table[i] += local_table[i];
        }
        semop(sem_id, &unlock, 1); // Déverrouillage


        printf("Processus %d : synchronisation cycle %d terminée\n", process_id, j + 1);

        memset(local_table, 0, ARRAY_SIZE * sizeof(int));
    }

    gettimeofday(&end, NULL);

    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("Processus %d : génération et synchronisation terminées en %.2f secondes\n", process_id, elapsed_time);

    free(local_table);
}

int main() {

    pid_t pid;
    int shm_id, sem_id;
    int *shared_table;

    shm_id = shmget(SHM_KEY, ARRAY_SIZE * sizeof(int), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Erreur shmget");
        exit(1);
    }

    shared_table = (int *)shmat(shm_id, NULL, 0);
    if (shared_table == (void *)-1) {
        perror("Erreur shmat");
        exit(1);
    }

    memset(shared_table, 0, ARRAY_SIZE * sizeof(int));

    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Erreur semget");
        exit(1);
    }

    semctl(sem_id, 0, SETVAL, 1);

    int sock = 0;
    struct sockaddr_in serv_addr;

    // Créer une socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur de création de la socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertir l'adresse IPv4 en format binaire
    if (inet_pton(AF_INET, IP_ADRESSE_INTERFACE_SERVEUR, &serv_addr.sin_addr) <= 0) {
        perror("Adresse du serveur invalide");
        return -1;
    }


    for (int p = 0; p < NUM_PROCESSES; p++) {
        pid = fork();
        if (pid == 0) {
            srand(time(NULL) * getpid() + clock()+444);
            generate_and_sync(p, shared_table, sem_id);
            shmdt(shared_table);
            close(sock);  // Fermer la socket dans le processus enfant
            exit(0);
        }
    }

    for (int p = 0; p < NUM_PROCESSES; p++) {
        wait(NULL);
    }

    printf("Distribution des nombres (10 premières cases) :\n");
    for (int i = 0; i < 10; i++) {
        printf("Nombre %d : %d occurrences\n", i, shared_table[i]);
    }




    // Se connecter au serveur
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Échec de la connexion au serveur");
        return -1;
    }

    printf("Connecté au serveur\n");



        int sent_size = 0;
        while (sent_size < ARRAY_SIZE * sizeof(int)) {
            int chunk_size = ARRAY_SIZE * sizeof(int) - sent_size > CHUNK_SIZE ? CHUNK_SIZE : ARRAY_SIZE * sizeof(int) - sent_size;
            int valwrite = send(sock, shared_table + sent_size / sizeof(int), chunk_size, 0);
            if (valwrite < 0) {
                perror("Erreur lors de l'envoi des données");
                break;
            }
            sent_size += valwrite;
        }

        // Recevoir la confirmation du serveur
        int valread, number;
        valread = read(sock, &number, sizeof(number));
        if (valread < 0) {
            perror("Erreur lors de la lecture de la confirmation");
        }
        printf("Confirmation reçue du serveur : %d\n", number);


    for (int i = 0; i < ARRAY_SIZE; i++) {
        totalOccurence+= shared_table[i];
    }

    create_data_file(shared_table, ARRAY_SIZE);

    shmdt(shared_table);
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    // Fermer la connexion
    close(sock);
    printf("Déconnecté du serveur\n");

    return 0;
}
