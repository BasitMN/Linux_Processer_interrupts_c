#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <setjmp.h>

jmp_buf recovery_point;
volatile atomic_int running = 1;
volatile atomic_int counter = 0;

void handle_sigint(int sig) {
    printf("\n[INTERRUPT] SIGINT - avslutar snallt...\n");
    running = 0;
}

void handle_sigusr1(int sig) {
    counter = 0;
    printf("\n[INTERRUPT] SIGUSR1 - raknare nollstalld\n");
}

void handle_sigfpe(int sig) {
    printf("\n[RECOVERY] fpe catched! Hoppa tillbaka!\n");
    longjmp(recovery_point, 1);
}

int main() {
    printf("Startar. PID = %d\n", getpid());
    printf("Skicka SIGUSR1 for att nollstalla: kill -USR1 %d\n", getpid());

    signal(SIGINT,  handle_sigint);
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGFPE,  handle_sigfpe);

    if (setjmp(recovery_point) == 0) {
        int x = 1 / 0;
    } else {
        printf("[INFO] Aterhamtning lyckad, fortsatter loop...\n");
    }

    while (running) {
        sleep(1);
        counter++;
        printf("Raknare: %d\n", counter);
    }

    printf("Avslutar snallt. Hej da!\n");
    return 0;
}
//test text bbbcc
