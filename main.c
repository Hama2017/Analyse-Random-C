#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int main() {
    for (int i = 0; i <= 1000000; i++) {
        srand(time(NULL));
        int a = rand()%1000;
        printf("%d - %d \n", i, a);
        if (a==1) {
            printf("ok");
            break;
        }
    }
}
