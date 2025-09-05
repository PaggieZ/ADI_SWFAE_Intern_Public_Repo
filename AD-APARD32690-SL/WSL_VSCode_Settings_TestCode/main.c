#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    printf("Hello from main.c\n");
    
    int count = 0;
    while(1) {
        printf("count: %d\n", count);
        count++;
        sleep(1);
    }
    return 0;
}