#include <stdio.h>

int main() {

    char *ptr = new char[256];
    printf("%d\n", ptr);
    delete[] ptr;
    printf("%d\n", ptr);

    return 0;
}