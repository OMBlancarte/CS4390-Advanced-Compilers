#include <stdio.h>

int main() {
    int sum = 0;
    int i, j;

    // Outer loop
    for (i = 0; i < 3; i++) {
        // Inner (nested) loop
        for (j = 0; j < 2; j++) {
            sum += i + j;
        }
    }

    printf("Sum = %d\n", sum);
    return 0;
}
