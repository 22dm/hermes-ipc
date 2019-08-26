//
// Created by nyako on 2019-08-25.
//

#include "ring.c"
#include <stdlib.h>

struct nsh_packet {
    int size;
    int id;
    char data[0];
};

char buffer_file[1000][104];
ring buffer_ring[1000];
int buffer_pos = 0;

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
        ring ring = init_ring(1000, file);
        buffer_ring[buffer_pos] = ring;
        buffer_pos++;
        return ring;
    }

    return NULL;
}

void nsh_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    get_ring(addr);
}

int nsh_recvfrom(int fd, void *buf, size_t buf_size, int state, const struct sockaddr *addr, socklen_t *addrlen) {
    ring ring = get_ring(addr);
    int data_size = *(int *) (ring + sizeof(struct ring_struct) + 4);
    printf("%d", data_size);
    int pkt_size = data_size + sizeof(struct nsh_packet);
    printf("%d", pkt_size);
    struct nsh_packet *pkt = malloc(pkt_size);
    int recv_size = 0;
    while (recv_size < pkt_size) {
        recv_size += recv_ring(ring, pkt + recv_size, pkt_size - recv_size);
    }
    int write_size = data_size < buf_size ? data_size : buf_size;
    printf("recv: %s\n", pkt->data);
    memcpy(buf, pkt->data, write_size);
    return write_size;
}

void nsh_sendto(int fd, const void *data, size_t size, int state, const struct sockaddr *addr, socklen_t addrlen) {
    ring ring = get_ring(addr);
    int pkt_size = sizeof(struct nsh_packet) + size;
    struct nsh_packet *pkt = malloc(pkt_size);
    pkt->size = size;
    memcpy(pkt->data, data, size);
    send_ring(ring, pkt, pkt_size);
}
