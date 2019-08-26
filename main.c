#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <zconf.h>
#include <sys/un.h>
#include "nsh.c"

char buf[1024];
int buf_size = 1024;

int sockfd;
struct sockaddr * addr;
socklen_t addrLen = sizeof(struct sockaddr_un);

void IOShmPre() {
    printf("pre\n");

    nsh_bind(sockfd, addr, addrLen);
}

void IOShmRun() {
    printf("run\n");

    printf("send\n");
    //发送
    const char *str = "123456789012345678901234567890";
    nsh_sendto(sockfd, str, strlen(str), 0, addr, addrLen);

    printf("recv\n");
    //接收
    int size = nsh_recvfrom(sockfd, buf, buf_size, 0, addr, &addrLen);
    buf[size] = 0;
    printf("%s", buf);
}

void IOShmClean(){
    printf("clean\n");
}


void IOUnixPre(){
    //发送句柄
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    unlink("/Users/nyako/echo.sock");
    bind(sockfd, addr, addrLen);
}

void IOUnixRun() {
    //发送
    const char *str = "123456789012345678901234567890";
    sendto(sockfd, str, strlen(str), 0, addr, addrLen);

    //接收
    int size = recvfrom(sockfd, buf, buf_size, 0, addr, &addrLen);
    size++;
}

void IOUnixClean(){
    //清理
    unlink("/Users/nyako/echo.sock");
    close(sockfd);
}

double benchmarkShm() {
    IOShmPre();
    clock_t start = clock();
    long times = 1000000;
    for (long i = 0; i < times; i++) {
        IOShmRun();
    }
    clock_t end = clock();
    double IOps = (double) times / ((double) (end - start) / CLOCKS_PER_SEC);
    IOShmClean();
    return IOps;
}

void testShm(){
    printf("test\n");
    IOShmPre();
    IOShmRun();
    IOShmClean();
}

double benchmarkUnix(){
    IOUnixPre();
    long times = 1000000;
    clock_t start = clock();
    for (long i = 0; i < times; i++) {
        IOUnixRun();
    }
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

    printf("1111\n");

    testShm();

    printf("2222\n");

    //printf("Ring: %f IO/s\n", benchmarkShm());
    //printf("Unix socket: %f IO/s\n", benchmarkUnix());
    return 0;
}