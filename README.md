# Étude Fonction rand()

Ce projet analyse le comportement de la fonction rand() en C à travers une architecture distribuée client-serveur. Il permet de générer et d'analyser une grande quantité de nombres aléatoires (jusqu'à 12000 milliards dans nos tests) en utilisant plusieurs ordinateurs en réseau.

## 🌟 Caractéristiques

### Architecture Distribuée
- Architecture client-serveur pour la distribution des calculs
- Support de multiples clients simultanés (testé avec 19 clients)
- Communication réseau via sockets TCP/IP
- Capacité de traitement massif (12000 milliards de nombres testés)

### Interface Principale
- Menu interactif pour lancer le client ou le serveur
- Validation automatique des adresses IP et ports
- Compilation automatique avec optimisation (-O3)
- Gestion robuste des erreurs de saisie
- Messages d'interface personnalisables via fichiers texte

### Performance
- Architecture multi-processus (6 processus par instance)
- Synchronisation via sémaphores
- Gestion de mémoire partagée (IPC)
- Traitement parallèle sur plusieurs machines

### Analyse & Statistiques
- Génération de fichiers CSV pour l'analyse
- Calcul des occurrences minimum et maximum
- Analyse des ratios de distribution
- Visualisation des données

## 📋 Prérequis

- Système d'exploitation Linux/Unix
- Compilateur GCC
- Réseau local pour le mode distribué

## 🛠️ Installation

1. Clonez le dépôt :
```bash
git clone https://github.com/Hama2017/Analyse-Random-C
cd Analyse-Random-C
```

2. Compilez les programmes :
```bash
gcc -o main main.c
```

## 💻 Utilisation

1. Lancez le programme principal :
```bash
./main
```

2. Suivez les instructions du menu pour :
    - Lancer un serveur en spécifiant l'adresse IP, le port et le nombre de clients
    - Ou lancer un client en spécifiant l'adresse IP et le port du serveur

### Configuration manuelle (alternative)
Si vous préférez compiler et exécuter les programmes individuellement :

1. Pour le serveur :
```bash
gcc -o server server.c 
./server [adresseIP:port] [nombreClients]
# Exemple : ./server 127.0.0.1:8080 19
```

2. Pour le client :
```bash
gcc -o client client.c 
./client [adresseIP:port]
# Exemple : ./client 127.0.0.1:8080
```

## 🔧 Configuration

### Paramètres Client
```c
#define TAILLE_TABLEAU (1<<28)  // Taille du tableau des occurrences
#define NBR_PROCESSUS 6         // Nombre de processus
#define NBR_CYCLES 10          // Nombre de cycles
#define NBR_RANDOMS (10000000000LL) // Nombres aléatoires par cycle
```

### Paramètres Serveur
```c
int nbrClientPrevus = 1;      // Nombre de clients à attendre
long long nbr_total_rand_generer = 1200000000000LL; // Nombre total de nombres à générer
```

## 📊 Structure du Projet

```
.
|__ main.c                # Programme principal (menu interactif)
├── server.c              # Code source du serveur
├── client.c              # Code source du client
├── log/                  # Dossiers des logs
│   ├── server/
│   │   └── log.txt      # Logs serveur
│   └── client/
│       └── log.txt      # Logs client
├── CSV/                  # Fichiers de données générés
│   ├── donnees_index_occurence.csv
│   └── donnees_ratio_max_min.csv
└── text/                 # Messages d'interface
    ├── main/
    │   └── text_bienvenue.txt
    ├── server/
    │   ├── text_bienvenue.txt
    │   └── text_fin.txt
    └── client/
        ├── text_bienvenue.txt
        └── text_fin.txt
```

## 🔍 Fonctionnement

### Programme Principal (main)
1. **Interface**
    - Affichage du menu de sélection
    - Validation des entrées utilisateur
    - Compilation automatique des programmes

2. **Configuration**
    - Validation des adresses IP
    - Vérification des ports (1-65535)
    - Limitation du nombre de clients (1-100)

### Serveur
1. **Initialisation**
    - Configuration du socket serveur
    - Création de la mémoire partagée et des sémaphores
    - Attente des connexions clients

2. **Traitement**
    - Réception des données des clients
    - Synchronisation des tableaux d'occurrences
    - Génération des statistiques globales

3. **Analyse**
    - Calcul des occurrences min/max
    - Génération des fichiers CSV
    - Production des ratios de distribution

### Client
1. **Génération**
    - Création des processus de génération
    - Production des nombres aléatoires
    - Synchronisation locale via sémaphores

2. **Communication**
    - Connexion au serveur
    - Envoi des données
    - Nettoyage des ressources

## 📝 Logs

Le système maintient des logs détaillés pour le serveur et le client :
- Horodatage de chaque opération
- Suivi des connexions réseau
- Statistiques de génération
- Erreurs et succès

## 📊 Fichiers CSV Générés

1. **donnees_index_occurence.csv**
    - Index des nombres générés
    - Nombre d'occurrences pour chaque valeur

2. **donnees_ratio_max_min.csv**
    - Ratio de distribution
    - Valeurs maximales et minimales
    - Statistiques globales

## 👥 Auteurs

- BA Hamadou
- BA Salimatouh Maliah

## 📅 Date de création

15 Décembre 2024

## ⚠️ Notes importantes

- Le programme principal doit être exécuté depuis la racine du projet
- Le programme a été testé avec succès sur 19 clients simultanés
- Capacité de génération de 12000 milliards de nombres aléatoires
- Utilisation optimale sur un réseau local pour les performances

## 📄 Licence

Ce projet est distribué sous licence MIT. Voir le fichier `LICENSE` pour plus de détails.