#include <netinet/in.h>
#include <arpa/inet.h>

const int tcp_buf_size = 8192;
char tcp_buf[8192];

extern long times;

int tcp_sendfd, tcp_recvfd;
int new_fd;

struct sockaddr * tcp_addr;
socklen_t tcp_addr_len = sizeof(struct sockaddr_in);

void tcp_pre(){

    //地址
    struct sockaddr_in * servaddr = calloc(1, sizeof(servaddr));
    servaddr->sin_family = AF_INET;
    servaddr->sin_port = htons(8080);
    servaddr->sin_addr.s_addr = inet_addr("127.0.0.1");
    tcp_addr = (struct sockaddr *) servaddr;

    //
    if ((tcp_sendfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }


    tcp_recvfd = socket(AF_INET, SOCK_STREAM, 0);


    if (bind(tcp_recvfd, tcp_addr, tcp_addr_len) < 0) {
        perror("bind error");
        exit(1);
    }

    if (listen(tcp_recvfd, 8) < 0) {
        perror("listen error");
        exit(1);
    }

    if (connect(tcp_sendfd, tcp_addr, tcp_addr_len) < 0) {
        perror("connect error");
        exit(-1);
    }

    if ((new_fd = accept(tcp_recvfd, NULL, NULL)) == -1) {
        perror("accpet error");
    }
}

void* tcp_send(){
    //发送
    //const char *str = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    char str[8192];

    for (long i = 0; i < times; i++) {
        send(tcp_sendfd, str, 8192, 0);
    }

    return NULL;
}

void* tcp_recv(){
    long send_size = times * 8192;
    int recv_size = 0;
    //接收
    while (recv_size < send_size){
        recv_size += recv(new_fd, tcp_buf, tcp_buf_size, 0);
    }
    printf("%d\n", recv_size);

    return NULL;
}

void tcp_run() {
    pthread_t send;
    pthread_create(&send, NULL, tcp_send, NULL);

    pthread_t recv;
    pthread_create(&recv, NULL, tcp_recv, NULL);

    pthread_join(send, NULL);
    pthread_join(recv, NULL);
}

void tcp_clean(){
    //清理
    close(tcp_recvfd);
    close(tcp_sendfd);
}

double tcp_benchmark(){
    tcp_clean();
    tcp_pre();
    clock_t start = clock();
    tcp_run();
    clock_t end = clock();
    double IOps = (double) times / ((double) (end - start) / CLOCKS_PER_SEC);
    tcp_clean();
    return IOps;
}

void tcp_test(){
    tcp_clean();
    tcp_pre();
    tcp_run();
    tcp_clean();
}
