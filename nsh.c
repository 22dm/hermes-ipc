//
// Created by nyako on 2019-08-25.
//

#include "ring.c"
#include <stdlib.h>

char buffer_file[1000][104];
ring buffer_ring[1000];
int buffer_pos = 0;

ring fd_ring[1024];
ring *fd_trans_ring[1024];
int fd_max_connect[1024];
int fds_pos = 0;

ring get_ring(const struct sockaddr *addr) {
    struct sockaddr_un *address = (struct sockaddr_un *) addr;
    const char *file = address->sun_path;
    int i;

    for (i = 0; i < buffer_pos; i++) {
        if (strcmp(buffer_file[i], file) == 0) {
            return buffer_ring[i];
        }
    }

    if (i == buffer_pos) {
        strcpy(buffer_file[buffer_pos], file);
        ring ring = init_ring(8192, file);
        buffer_ring[buffer_pos] = ring;
        buffer_pos++;
        return ring;
    }

    return NULL;
}

int nsh_socket() {
    return fds_pos++;
}

void nsh_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    fd_ring[fd] = get_ring(addr);
}

void nsh_listener(int fd, int max_connect) {

}

void* nsh_accepter(int fd, struct sockaddr *addr, socklen_t *addr_len) {

}

void nsh_accept(int fd, struct sockaddr *addr, socklen_t *addr_len) {
    ring listen_ring = fd_ring[fd];
    ring* trans_ring = fd_trans_ring[fd];
    int max_connect = fd_max_connect[fd];

    while (listen_ring != NULL) {
        for(int i = 0; i < max_connect; i++){
            if(trans_ring[i] == NULL){
                recv_ring(listen_ring, trans_ring + i, sizeof(ring));
                int trans_fd = nsh_socket();
                nsh_bind(trans_fd);
                pthread_t accepter;
                pthread_create(&accepter, NULL, nsh_accepter, NULL);
            }
        }
    }
}

void nsh_listen(int fd, int max_connect) {
    ring *trans_ring = fd_trans_ring[fd];

    fd_max_connect[fd] = max_connect;
    fd_trans_ring[fd] = malloc(sizeof(ring) * max_connect);
    for (int i = 0; i < max_connect; i++) {
        trans_ring[i] = NULL;
    }
}

int nsh_recvfrom(int fd, void *buf, size_t buf_size, int state, const struct sockaddr *addr, socklen_t *addrlen) {
    ring ring = get_ring(addr);
    int data_size;
    //while(ring->read_offset == ring->write_offset){
    //}
    //data_size = *((int *)ring + 3);
    //data_size = 3720000;
    struct nsh_packet *pkt = malloc(sizeof(struct nsh_packet));

    recv_ring_header(ring, pkt);

    //int pkt_size = pkt->size;
    int pkt_size = 372;
    int recv_size = 0;
    while (recv_size < pkt_size) {
        recv_size += recv_ring(ring, (void *) buf + recv_size, pkt_size - recv_size);
    }
    int write_size = data_size < buf_size ? data_size : buf_size;
    return write_size;
}

void nsh_sendto(int fd, const void *data, size_t size, int state, const struct sockaddr *addr, socklen_t addrlen) {
    ring ring = get_ring(addr);

    struct nsh_packet *pkt = malloc(sizeof(struct nsh_packet));
    pkt->size = size;
    send_ring_header(ring, pkt);

    send_ring(ring, data, size);
}
