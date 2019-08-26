#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <zconf.h>
#include <sys/un.h>
#include <pthread.h>

//#define DEBUG

#include "nsh.c"

char buf[1024];
int buf_size = 1024;

int times = 100000;

int sockfd;
struct sockaddr * addr;
socklen_t addrLen = sizeof(struct sockaddr_un);

void IOShmPre() {
    shm_unlink("/Users/nyako/echo.sock");
    nsh_bind(sockfd, addr, addrLen);
}

void* IOShmSend(){
    //发送
    const char *str = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (long i = 0; i < times; i++) {
        nsh_sendto(sockfd, str, strlen(str), 0, addr, addrLen);
    }
}

void* IOShmRecv(){
    //接收
    for (long i = 0; i < times; i++) {
        int size = nsh_recvfrom(sockfd, buf, buf_size, 0, addr, &addrLen);
    }
    //printf("%s", buf);
};

void IOShmRun() {

    pthread_t send;
    pthread_create(&send, NULL, IOShmSend, NULL);

    pthread_t recv;
    pthread_create(&recv, NULL, IOShmRecv, NULL);

    pthread_join(send, NULL);
    pthread_join(recv, NULL);
}

void IOShmClean(){
}

int sendfd, recvfd;

void IOUnixPre(){
    //发送句柄
    sendfd = socket(AF_UNIX, SOCK_STREAM, 0);
    recvfd = socket(AF_UNIX, SOCK_STREAM, 0);
    unlink("/Users/nyako/echo.sock");
    bind(recvfd, addr, addrLen);
}

void* IOUnixSend(){
    //发送
    const char *str = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (long i = 0; i < times; i++) {
        sendto(sendfd, str, strlen(str), 0, addr, addrLen);
    }
}

void* IOUnixRecv(){
    //接收
    for (long i = 0; i < times; i++) {
        int size = recvfrom(recvfd, buf, buf_size, 0, addr, &addrLen);
    }
}

void IOUnixRun() {
    pthread_t send;
    pthread_create(&send, NULL, IOUnixSend, NULL);

    pthread_t recv;
    pthread_create(&recv, NULL, IOUnixRecv, NULL);

    pthread_join(send, NULL);
    pthread_join(recv, NULL);

}

void IOUnixClean(){
    //清理
    unlink("/Users/nyako/echo.sock");
    close(sockfd);
}

double benchmarkShm() {
    IOShmPre();
    clock_t start = clock();
    IOShmRun();
    clock_t end = clock();
    double IOps = (double) times / ((double) (end - start) / CLOCKS_PER_SEC);
    IOShmClean();
    return IOps;
}

void testShm(){
    IOShmPre();
    IOShmRun();
    IOShmClean();
}

double benchmarkUnix(){
    IOUnixPre();
    clock_t start = clock();
    IOUnixRun();
    clock_t end = clock();
    double IOps = (double) times / ((double) (end - start) / CLOCKS_PER_SEC);
    IOUnixClean();
    return IOps;
}

void testUnix(){
    IOUnixPre();
    IOUnixRun();
    IOUnixClean();
}

int main() {

    //设置地址
    struct sockaddr_un * servaddr = calloc(1, sizeof(struct sockaddr_un));
    servaddr->sun_family = AF_UNIX;
    strcpy(servaddr->sun_path, "/Users/nyako/echo.sock");
    addr = (struct sockaddr *) servaddr;

    //testShm();

    printf("        Shm: %f IO/s\n", benchmarkShm());
    printf("Unix socket: %f IO/s\n", benchmarkUnix());
    printf("        Shm: %f IO/s\n", benchmarkShm());
    printf("Unix socket: %f IO/s\n", benchmarkUnix());
    printf("        Shm: %f IO/s\n", benchmarkShm());
    printf("Unix socket: %f IO/s\n", benchmarkUnix());
    printf("        Shm: %f IO/s\n", benchmarkShm());
    printf("Unix socket: %f IO/s\n", benchmarkUnix());
    printf("        Shm: %f IO/s\n", benchmarkShm());
    printf("Unix socket: %f IO/s\n", benchmarkUnix());
    printf("        Shm: %f IO/s\n", benchmarkShm());
    printf("Unix socket: %f IO/s\n", benchmarkUnix());
    return 0;
}