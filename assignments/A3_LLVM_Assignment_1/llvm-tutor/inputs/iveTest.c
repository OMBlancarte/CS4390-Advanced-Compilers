// iveTest.c
#include <stdio.h>

void f(int x) {
    printf("%d\n", x);
}

int main() {
    int a[100];
    for (int i = 0; i < 100; ++i) {
        a[i] = i * 2;
    }

    for (int i = 0; i < 100; ++i) {
        f(a[i]);
    }

    return 0;
}
