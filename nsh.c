//
// Created by nyako on 2019-08-25.
//

#include "ring.c"
#include <stdlib.h>

#define SHM_POOL_SIZE 1024

char buffer_file[1000][104];
ring buffer_ring[1000];
int buffer_pos = 0;

struct shm_socket_struct {
    int parent_fd;
    int *child_fd;
    char *addr_str;
    int max_connect;
    ring send_ring;
    ring recv_ring;
};

typedef struct shm_socket_struct *shm_socket;

shm_socket shm_sockets[1024];
int fds_pos = 0;

shm_socket shm_socket_init() {
    shm_socket shm_socket_obj = malloc(sizeof(struct shm_socket_struct));
    shm_socket_obj->addr_str = NULL;
    shm_socket_obj->send_ring = NULL;
    shm_socket_obj->recv_ring = NULL;
    shm_socket_obj->child_fd = NULL;
    shm_socket_obj->parent_fd = -1;
    shm_socket_obj->max_connect = -1;
    return shm_socket_obj;
}

int nsh_socket() {
    return fds_pos++;
}

int nsh_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    shm_sockets[fd] = shm_socket_init();
    shm_socket shm_socket_obj = shm_sockets[fd];
    shm_socket_obj->addr_str = malloc(100);
    strcpy(shm_socket_obj->addr_str, ((struct sockaddr_un *) addr)->sun_path);
    shm_socket_obj->recv_ring = init_ring(fd);
    return 0;
}

int nsh_accept(int fd, struct sockaddr *addr, socklen_t *addr_len) {
    shm_socket shm_socket_obj = shm_sockets[fd];
    if (shm_socket_obj == NULL) {
        return -1;
    }

    int *child_fds = shm_socket_obj->child_fd;
    int max_connect = shm_socket_obj->max_connect;
    ring parent_recv_ring = shm_socket_obj->recv_ring;

    for (int i = 0; i < max_connect; i++) {
        //遍历链接池
        if (child_fds[i] == -1) {
            //如果有空位
            ring child_send_ring, child_recv_ring;
            recv_ring(parent_recv_ring, &child_send_ring, sizeof(ring));
            //建立新的 fd 并记录
            int child_fd = nsh_socket();
            child_fds[i] = child_fd;
            //建立新的 socket 对象
            shm_sockets[child_fd] = shm_socket_init();
            shm_socket shm_child_socket = shm_sockets[child_fd];
            shm_child_socket->parent_fd = fd;
            shm_child_socket->send_ring = child_send_ring;
            //建立接收环并发送
            child_recv_ring = init_ring(child_fd);
            shm_child_socket->recv_ring = child_recv_ring;

            for (int j = 0; j < SHM_POOL_SIZE; j++) {
                shm_socket shm_client_socket_obj = shm_sockets[i];
                if (shm_client_socket_obj == NULL || shm_client_socket_obj->recv_ring == NULL) {
                    continue;
                }
                if (shm_client_socket_obj->recv_ring == child_send_ring) {
                    shm_client_socket_obj->send_ring = child_recv_ring;
                    return child_fd;
                }
            }
            return -3;
        }
    }
    return -2;
}

/*
 * 在本设计中，listen 的主要作用为，确定最大连接数，初始化相关内容
 */
int nsh_listen(int fd, int max_connect) {
    shm_socket shm_socket_obj = shm_sockets[fd];
    if (shm_socket_obj == NULL) {
        return -1;
    }

    //填入最大连接数
    shm_socket_obj->child_fd = malloc(sizeof(int) * max_connect);
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
    shm_socket shm_socket_obj = shm_socket_init();
    shm_sockets[fd] = shm_socket_obj;
    ring shm_recv_ring = init_ring(fd);
    shm_socket_obj->recv_ring = shm_recv_ring;
    for (int i = 0; i < SHM_POOL_SIZE; i++) {
        shm_socket shm_server_socket_obj = shm_sockets[i];
        if (shm_server_socket_obj == NULL || shm_server_socket_obj->addr_str == NULL) {
            continue;
        }
        if (strcmp(shm_server_socket_obj->addr_str, ((struct sockaddr_un *) addr)->sun_path) == 0) {
            send_ring(shm_server_socket_obj->recv_ring, &shm_recv_ring, sizeof(shm_recv_ring));
            return 0;
        }
    }
    return -1;
}

int nsh_recv(int fd, void *buf, size_t buf_size, int type) {
    shm_socket shm_socket_obj = shm_sockets[fd];
    int recv_size = 0, total_recv_size = 0;
    do {
        recv_size = recv_ring(shm_socket_obj->recv_ring, buf, buf_size);
        total_recv_size += recv_size;
    } while (recv_size > 0);
    return total_recv_size;
}

void nsh_send(int fd, const void *data, size_t size, int type) {
    shm_socket shm_socket_obj = shm_sockets[fd];
    send_ring(shm_socket_obj->send_ring, data, size);
}
