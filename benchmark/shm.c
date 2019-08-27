const int shm_buf_size = 8192;
char shm_buf[shm_buf_size];

extern int times;

int shm_sendfd, shm_recvfd;
int new_fd;

struct sockaddr * shm_addr;
socklen_t shm_addr_len = sizeof(struct sockaddr_un);
const char *shm_file = "/Users/nyako/echo.sock";

void shm_pre() {

    //地址
    struct sockaddr_un * servaddr = calloc(1, sizeof(struct sockaddr_un));
    servaddr->sun_family = AF_UNIX;
    strcpy(servaddr->sun_path, shm_file);
    shm_addr = (struct sockaddr *) servaddr;

    if ((shm_sendfd = nsh_socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }


    shm_recvfd = nsh_socket(AF_UNIX, SOCK_STREAM, 0);


    unlink(shm_file);


    if (nsh_bind(shm_recvfd, shm_addr, shm_addr_len) < 0) {
        perror("bind error");
        exit(1);
    }

    if (nsh_listen(shm_recvfd, 8) < 0) {
        perror("listen error");
        exit(1);
    }

    if (nsh_connect(shm_sendfd, shm_addr, shm_addr_len) < 0) {
        perror("connect error");
        exit(-1);
    }

    if ((new_fd = nsh_accept(shm_recvfd, NULL, NULL)) == -1) {
        perror("accpet error");
    }
}

void* shm_send(){
    //发送
    const char *str = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (long i = 0; i < times; i++) {
        nsh_send(shm_sendfd, str, strlen(str), 0);
    }
}

void* shm_recv(){
    int send_size = times * 372;
    int recv_size = 0;
    //接收
    while (recv_size != send_size){
        recv_size += nsh_recv(new_fd, shm_buf, shm_buf_size, 0);
    }
    printf("%d %s\n", strlen(shm_buf), shm_buf);
}

void shm_run() {

    pthread_t send;
    pthread_create(&send, NULL, shm_send, NULL);

    pthread_t recv;
    pthread_create(&recv, NULL, shm_recv, NULL);

    pthread_join(send, NULL);
    pthread_join(recv, NULL);
}

void shm_clean(){
    shm_unlink(shm_file);
}

double shm_benchmark() {
    shm_pre();
    clock_t start = clock();
    shm_run();
    clock_t end = clock();
    double IOps = (double) times / ((double) (end - start) / CLOCKS_PER_SEC);
    shm_clean();
    return IOps;
}

void shm_test(){
    shm_pre();
    shm_run();
    shm_clean();
}