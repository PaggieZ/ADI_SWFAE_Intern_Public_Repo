#include <stdio.h>
#include "add.h"


struct numPair {
    int n1;
    int n2;
};

int main() {
    struct numPair NP;
    NP.n1 = 1;
    NP.n2 = 2;

    printf("num1: %d, num2: %d\n", NP.n1, NP.n2);

    int sum = add(NP.n1, NP.n2);

    printf("sum: %d", sum);

    return 0;
}
