#include "constValue.h"

char unix_buf[TEST_BUF_SIZE];

extern long times;

int unix_sendfd, unix_recvfd;
int new_fd;

struct sockaddr * unix_addr;
socklen_t unix_addr_len = sizeof(struct sockaddr_un);
const char *unix_file = "/Users/nyako/echo.sock";

void unix_pre(){

    //地址
    struct sockaddr_un * servaddr = calloc(1, sizeof(servaddr));
    servaddr->sun_family = AF_UNIX;
    strcpy(servaddr->sun_path, unix_file);
    unix_addr = (struct sockaddr *) servaddr;

    //
    if ((unix_sendfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }


    unix_recvfd = socket(AF_UNIX, SOCK_STREAM, 0);


    unlink(unix_file);

    if (bind(unix_recvfd, unix_addr, unix_addr_len) < 0) {
        perror("bind error");
        exit(1);
    }

    if (listen(unix_recvfd, 8) < 0) {
        perror("listen error");
        exit(1);
    }

    if (connect(unix_sendfd, unix_addr, unix_addr_len) < 0) {
        perror("connect error");
        exit(-1);
    }

    if ((new_fd = accept(unix_recvfd, NULL, NULL)) == -1) {
        perror("accpet error");
    }
}

void* unix_send(){
    //发送
    //const char *str = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    char str[TEST_DATA_SIZE];

    for (long i = 0; i < times; i++) {
        send(unix_sendfd, (void *)str, TEST_DATA_SIZE, 0);
    }

    return NULL;
}

void* unix_recv(){
    long send_size = times * TEST_DATA_SIZE;
    int recv_size = 0;
    //接收
    while (recv_size < send_size){
        recv_size += recv(new_fd, unix_buf, TEST_BUF_SIZE, 0);
    }
    printf("%d\n", recv_size);

    return NULL;
}

void unix_run() {
    pthread_t send;
    pthread_create(&send, NULL, unix_send, NULL);

    pthread_t recv;
    pthread_create(&recv, NULL, unix_recv, NULL);

    pthread_join(send, NULL);
    pthread_join(recv, NULL);
}

void unix_clean(){
    //清理
    unlink(unix_file);
    close(unix_recvfd);
    close(unix_sendfd);
}

double unix_benchmark(){
    unix_clean();
    unix_pre();
    clock_t start = clock();
    unix_run();
    clock_t end = clock();
    double IOps = (double) times / ((double) (end - start) / CLOCKS_PER_SEC);
    unix_clean();
    return IOps;
}

void unix_test(){
    unix_clean();
    unix_pre();
    unix_run();
    unix_clean();
}
