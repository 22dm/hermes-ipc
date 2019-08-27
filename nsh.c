//
// Created by nyako on 2019-08-25.
//

#include "ring.c"
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>

#define SHM_POOL_SIZE 1024

struct shm_socket_struct {
    int valid;
    int parent_fd;
    int child_fd[64];
    char addr_str[100];
    int max_connect;
    int send_fd;
    struct ring_struct recv_ring_struct;
};

typedef struct shm_socket_struct *shm_socket;

int* fds_pos_ptr;

void* shm_base;
const char* shm_base_file = "/Users/nyako/shm_base22dm";

shm_socket get_socket(int fd){
    return (shm_socket)(shm_base + sizeof(int) + fd * sizeof(struct shm_socket_struct));
}

shm_socket shm_socket_init(int fd) {
    shm_socket shm_socket_obj = get_socket(fd);
    shm_socket_obj->valid = 1;
    shm_socket_obj->send_fd = -1;
    shm_socket_obj->parent_fd = -1;
    shm_socket_obj->max_connect = -1;
    shm_socket_obj->recv_ring_struct.size = 8192;
    shm_socket_obj->recv_ring_struct.read_offset = 0;
    shm_socket_obj->recv_ring_struct.write_offset = 0;
    return shm_socket_obj;
}

int nsh_socket() {
    int new_file = 0;
    int metadata_size = sizeof(int) + sizeof(struct shm_socket_struct) * SHM_POOL_SIZE;
    int file_fd = shm_open(shm_base_file, O_CREAT | O_RDWR | O_EXCL, 0777);
    if (file_fd < 0) { // 对象已存在
        file_fd = shm_open(shm_base_file, O_RDWR, 0777);
    } else { // 新对象，设置大小
        int aa = ftruncate(file_fd, metadata_size);
        new_file = 1;
    }
    void* metadata = mmap(NULL, metadata_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0);
    close(file_fd);

    fds_pos_ptr = (int*) metadata;
    shm_base = metadata + 1;

    if(new_file == 1){
        *fds_pos_ptr = 0;
    }

    return (*fds_pos_ptr)++;
}

int nsh_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    shm_socket_init(fd);
    shm_socket shm_socket_obj = get_socket(fd);
    strcpy(shm_socket_obj->addr_str, ((struct sockaddr_un *) addr)->sun_path);
    return 0;
}

int nsh_accept(int fd, struct sockaddr *addr, socklen_t *addr_len) {
    shm_socket shm_socket_obj = get_socket(fd);
    if (shm_socket_obj->valid == 0) {
        return -1;
    }

    int *child_fds = shm_socket_obj->child_fd;
    int max_connect = shm_socket_obj->max_connect;

    for (int i = 0; i < max_connect; i++) {
        //遍历链接池
        if (child_fds[i] == -1) {
            //如果有空位
            int child_send_fd;
            recv_ring(&(shm_socket_obj->recv_ring_struct), &child_send_fd, sizeof(int));
            //建立新的 fd 并记录
            int child_fd = nsh_socket();
            child_fds[i] = child_fd;
            //建立新的 socket 对象
            shm_socket_init(child_fd);
            shm_socket shm_child_socket = get_socket(child_fd);
            shm_child_socket->parent_fd = fd;
            shm_child_socket->send_fd = child_send_fd;
            //建立接收环并发送
            get_socket(child_send_fd)->send_fd = child_fd;
            return child_fd;
        }
    }
    return -2;
}

/*
 * 在本设计中，listen 的主要作用为，确定最大连接数，初始化相关内容
 */
int nsh_listen(int fd, int max_connect) {
    shm_socket shm_socket_obj = get_socket(fd);
    if (shm_socket_obj->valid == 0) {
        return -1;
    }

    //填入最大连接数
    shm_socket_obj->max_connect = max_connect;

    //子句柄列表初始化为 -1
    int *child_fds = shm_socket_obj->child_fd;
    for (int i = 0; i < max_connect; i++) {
        child_fds[i] = -1;
    }

    return 0;
}

/*
 * 在本设计中，listen 的主要作用为，创建接收内存，并于 server 端交换内存
 */

int nsh_connect(int fd, struct sockaddr *addr, socklen_t *addr_len) {
    shm_socket_init(fd);
    shm_socket shm_socket_obj = get_socket(fd);
    for (int i = 0; i < SHM_POOL_SIZE; i++) {
        shm_socket shm_server_socket_obj = get_socket(i);
        if (shm_socket_obj->valid == 0) {
            continue;
        }
        if (strcmp(shm_server_socket_obj->addr_str, ((struct sockaddr_un *) addr)->sun_path) == 0) {
            send_ring(&(shm_server_socket_obj->recv_ring_struct), &fd, sizeof(int));
            return 0;
        }
    }
    return -1;
}

int nsh_recv(int fd, void *buf, size_t buf_size, int type) {
    shm_socket shm_socket_obj = get_socket(fd);
    int recv_size = 0, total_recv_size = 0;
    do {
        recv_size = recv_ring(&(shm_socket_obj->recv_ring_struct), buf + total_recv_size, buf_size - total_recv_size);
        total_recv_size += recv_size;
    } while (recv_size > 0);
    return total_recv_size;
}

void nsh_send(int fd, const void *data, size_t size, int type) {
    shm_socket shm_socket_obj = get_socket(fd);
    send_ring(&(get_socket(shm_socket_obj->send_fd)->recv_ring_struct), data, size);
}
