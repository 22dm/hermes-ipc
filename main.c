#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <zconf.h>
#include <sys/un.h>
#include <pthread.h>

#include "nsh.c"
#include "benchmark/unix.c"
#include "benchmark/shm.c"
#include "benchmark/tcp.c"

int times = 1000000;

int main() {
    printf("loopback TCP               : %f IO/s\n", tcp_benchmark());
    printf("Unix domain socket (stream): %f IO/s\n", unix_benchmark());
    printf("Shared memory IPC          : %f IO/s\n", shm_benchmark());
    return 0;
}