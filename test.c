#include <stdio.h>
#include <stdlib.h>

#define N (1 << 28)   // Taille originale du tableau
#define K 1024        // Taille d'un bloc pour le pliage

int main() {
    unsigned int *tab = malloc(N * sizeof(unsigned int));
    if (!tab) {
        perror("Erreur allocation mémoire");
        return EXIT_FAILURE;
    }

    // Remplir le tableau avec des valeurs quelconques
    for (unsigned int i = 0; i < N; i++) {
        tab[i] = i % 256; // Exemple : les indices répètent des valeurs de 0 à 255
    }

    unsigned int new_size = N / K; // Taille réduite
    unsigned int *reduced_tab = malloc(new_size * sizeof(unsigned int));
    if (!reduced_tab) {
        perror("Erreur allocation mémoire");
        free(tab);
        return EXIT_FAILURE;
    }

    // Pliage : somme des valeurs dans des blocs de taille K
    for (unsigned int i = 0; i < new_size; i++) {
        unsigned int sum = 0;
        for (unsigned int j = 0; j < K; j++) {
            sum += tab[i * K + j];
        }
        reduced_tab[i] = sum; // Stocke la somme ou une autre statistique
    }

    // Exemple de vérification ou de traçage
    for (unsigned int i = 0; i < new_size; i++) {
        printf("%u %u\n", i, reduced_tab[i]); // x = indice réduit, y = valeur du bloc
    }

    // Libération de la mémoire
    free(tab);
    free(reduced_tab);

    return 0;
}
