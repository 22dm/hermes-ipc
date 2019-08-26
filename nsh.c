//
// Created by nyako on 2019-08-25.
//

#include "ring.c"
#include <stdlib.h>

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
        ring ring = init_ring(6553600, file);
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
    printf("aaaa");

    ring ring = get_ring(addr);
    int data_size;
    //while(ring->read_offset == ring->write_offset){
    //}
    //data_size = *((int *)ring + 3);
    //data_size = 3720000;
    struct nsh_packet *pkt = malloc(sizeof(struct nsh_packet));

    recv_ring_header(ring, pkt);
    printf("aaaa");

    //int pkt_size = pkt->size;
    int pkt_size = 3720000;
    int recv_size = 0;
    while (recv_size < pkt_size) {
        recv_size += recv_ring(ring, (void *) buf + recv_size, pkt_size - recv_size);
    }
    int write_size = data_size < buf_size ? data_size : buf_size;
    return write_size;
}

void nsh_sendto(int fd, const void *data, size_t size, int state, const struct sockaddr *addr, socklen_t addrlen) {
    printf("aaaa");

    ring ring = get_ring(addr);
    printf("aaaa");

    int pkt_size = sizeof(struct nsh_packet);
    struct nsh_packet *pkt = malloc(pkt_size);
    pkt->size = size;
    send_ring_header(ring, pkt);
    printf("aaaa");

    send_ring(ring, pkt, pkt_size);
    printf("aaaa");

}
