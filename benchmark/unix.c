char unix_buf[37200000];
int unix_buf_size = 37200000;

extern int times;

int unix_sendfd, unix_recvfd;

struct sockaddr * unix_addr;
socklen_t unix_addr_len = sizeof(struct sockaddr_un);
const char *unix_file = "/Users/nyako/echo.sock";

void unix_pre(){
    struct sockaddr_un * servaddr = calloc(1, sizeof(struct sockaddr_un));
    servaddr->sun_family = AF_UNIX;
    strcpy(servaddr->sun_path, "/Users/nyako/echo.sock");
    unix_addr = (struct sockaddr *) servaddr;

    //发送句柄
    unix_sendfd = socket(AF_UNIX, SOCK_STREAM, 0);
    unix_recvfd = socket(AF_UNIX, SOCK_STREAM, 0);
    unlink(unix_file);
    bind(unix_recvfd, unix_addr, unix_addr_len);
}

void* unix_send_big(){
    //发送
    char *data = malloc(3720001);
    const char *str = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";


    for(int i = 0; i < 10000; i++){
        memcpy(data + i * 372, str, 372);
    }

    data[3720000] = 0;

    for (long i = 0; i < times; i++) {
        struct nsh_packet *pkt = malloc(3720000);
        memcpy(pkt, data, 3720000);
        sendto(unix_sendfd, data, 3720000, 0, unix_addr, unix_addr_len);
    }
}

void* unix_send(){
    //发送
    const char *str = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (long i = 0; i < times; i++) {
        sendto(unix_sendfd, str, strlen(str), 0, unix_addr, unix_addr_len);
    }
}

void* unix_recv(){
    //接收
    for (long i = 0; i < times; i++) {
        int size = recvfrom(unix_recvfd, unix_buf, unix_buf_size, 0, unix_addr, &unix_addr_len);
        struct nsh_packet *pkt = malloc(3720000);
        memcpy(pkt, unix_buf, 3720000);
    }
}

void unix_run() {
    pthread_t send;
    pthread_create(&send, NULL, unix_send_big, NULL);

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
    unix_pre();
    clock_t start = clock();
    unix_run();
    clock_t end = clock();
    double IOps = (double) times / ((double) (end - start) / CLOCKS_PER_SEC);
    unix_clean();
    return IOps;
}

void unix_test(){
    unix_pre();
    unix_run();
    unix_clean();
}