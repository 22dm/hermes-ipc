char shm_buf[1024];
int shm_buf_size = 1024;

extern int times;

int shm_fd;

struct sockaddr * shm_addr;
socklen_t shm_addr_len = sizeof(struct sockaddr_un);
const char *shm_file = "/Users/nyako/echo.sock";

void shm_pre() {
    struct sockaddr_un * servaddr = calloc(1, sizeof(struct sockaddr_un));
    servaddr->sun_family = AF_UNIX;
    strcpy(servaddr->sun_path, "/Users/nyako/echo.sock");
    shm_addr = (struct sockaddr *) servaddr;

    shm_unlink(shm_file);
    nsh_bind(shm_fd, shm_addr, shm_addr_len);
}

void* shm_send(){
    //发送
    const char *str = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (long i = 0; i < times; i++) {
        nsh_sendto(shm_fd, str, strlen(str), 0, shm_addr, shm_addr_len);
    }
}

void* shm_recv(){
    //接收
    for (long i = 0; i < times; i++) {
        int size = nsh_recvfrom(shm_fd, shm_buf, shm_buf_size, 0, shm_addr, &shm_addr_len);
    }
    printf("%s", shm_buf);
};

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