#include <stdio.h>

void up() {
    static int i = 0;
    int j = 0;

    i++;
    j++;

    printf("%d vs %d\n", i, j);
}
