#include <iostream>
#include <time.h>
#include "uthreads.h"

void f(void) {
    std::cout << "~f\n";

    clock_t ticks1, ticks2;
    int i;

    for (i=0; i<30; ++i) {
        std::cout << "~f("<<i<<")\n";

        ticks1=clock();
        ticks2=ticks1;

        while ((ticks2/CLOCKS_PER_SEC-ticks1/CLOCKS_PER_SEC)<1)
            ticks2=clock();



        std::cout << "~f<\n";
        std::cout << "~f<<" << i << "\n";

    }

    uthread_terminate(uthread_get_tid());

    std::cout << "f~fucking shit\n";
}

void g(void) {
    std::cout << "~g\n";

    clock_t ticks1, ticks2;
    int i;

    for (i=0; i<30; ++i) {
        std::cout << "~g("<<i<<")\n";


        ticks1=clock();
        ticks2=ticks1;

        while ((ticks2/CLOCKS_PER_SEC-ticks1/CLOCKS_PER_SEC)<1)
            ticks2=clock();


        std::cout << "~g<\n";
        std::cout << "~g<<"<<i<<"\n";

    }

    uthread_terminate(uthread_get_tid());

    std::cout << "g~fucking shit\n";
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