#include <iostream>
#include <time.h>
#include "uthreads.h"

void f(void) {
    std::cout << "~f\n";

    clock_t ticks1, ticks2;
    int i;

    for (i=0; i<30; ++i) {

        ticks1=clock();
        ticks2=ticks1;

        while ((ticks2/CLOCKS_PER_SEC-ticks1/CLOCKS_PER_SEC)<1)
            ticks2=clock();


        std::cout << "f=" << i <<  '|' << uthread_get_quantums(uthread_get_tid()) << std::endl;

    }
}

void g(void) {
    std::cout << "~g\n";

    clock_t ticks1, ticks2;
    int i;

    for (i=0; i<30; ++i) {

        ticks1=clock();
        ticks2=ticks1;

        while ((ticks2/CLOCKS_PER_SEC-ticks1/CLOCKS_PER_SEC)<1)
            ticks2=clock();


        std::cout << "g=" << i <<  '|' << uthread_get_quantums(uthread_get_tid()) << std::endl;

    }
}

int main() {

    int prio[] = {500, 250, 500};


    uthread_init(prio, 3);

    uthread_spawn(f, 1);
    uthread_spawn(g, 2);

    std::cout << "~1\n";
    for (;;) {}

    return 0;
}