#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <zconf.h>
#include <sys/un.h>
#include <pthread.h>

//#define DEBUG

#include "nsh.c"
#include "benchmark/shm.c"
//#include "benchmark/unix.c"
//#include "benchmark/tcp.c"

int times = 1;

int main() {
    printf("aaaa");

    //shm_test();

    //printf("Unix socket: %f IO/s\n", unix_benchmark());
    printf("        Shm: %f IO/s\n", shm_benchmark());
    return 0;
}