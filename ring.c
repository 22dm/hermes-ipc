//
// Created by nyako on 2019-08-25.
//

#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>

struct ring_struct {
    int read_offset; //下一次从哪里开始读
    int write_offset; //下一次从哪里开始写
    int size; //环形队列大小，一次的最大写入量为 size - 1
    char data[0]; //环形队列
};

struct nsh_packet_header {
    int size;
    int id;
};

typedef struct ring_struct *ring;

ring init_ring(int size, const char *file) {
    int fd = shm_open(file, O_CREAT | O_RDWR | O_EXCL, 0777);
    if (fd < 0) {
        // 对象已存在
        fd = shm_open(file, O_RDWR, 0777);

    } else {
        // 新对象，设置大小
        ftruncate(fd, size + sizeof(struct ring_struct));
    }

    ring ring = mmap(NULL, size + sizeof(struct ring_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    ring->size = size;
    ring->read_offset = 0;
    ring->write_offset = 0;
    return ring;
}

//读取数据，只读一次
int recv_ring(ring ring, char *buf, int buf_size) {
    int read_size;
    int max_read;
    int ring_size = ring->size;
    while (ring->read_offset == ring->write_offset) {
    }
    int read_offset = ring->read_offset;
    int write_offset = ring->write_offset;
    if (read_offset < write_offset) {
        //往后面读一点就可以
        max_read = write_offset - read_offset;
    } else {
        //可以读到结尾
        max_read = ring_size - read_offset;
    }
    read_size = max_read > buf_size ? buf_size : max_read;
    memcpy(buf, ring->data + read_offset, read_size);
    if (ring->read_offset + read_size == ring_size) {
        ring->read_offset = 0;
    } else {
        ring->read_offset += read_size;
    }
    return read_size;
}

//向环中写入数据
int send_ring(ring ring, const char *buf, int size) {
    int read_offset;
    int write_offset;
    int ring_size = ring->size;
    int max_write;
    int write_size;

    while (size > 0) {
        read_offset = ring->read_offset;
        write_offset = ring->write_offset;

        if ((read_offset - write_offset + ring_size) % ring_size == 1) {
            continue;
        }

        if (write_offset < read_offset) {
            max_write = read_offset - write_offset - 1;
        } else {
            if (read_offset == 0) {
                max_write = ring->size - ring->write_offset - 1;
            } else {
                max_write = ring->size - ring->write_offset;
            }
        }

        write_size = max_write < size ? max_write : size;
        memcpy(ring->data + ring->write_offset, buf, write_size);
        int after_write_offset = ring->write_offset + write_size;
        if(after_write_offset == ring_size) {
            ring->write_offset = 0;
        } else {
            ring->write_offset += write_offset;
        }
        size -= write_size;
        buf += write_size;
    }
    return 0;
}
