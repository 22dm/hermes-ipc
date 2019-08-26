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
    int read_offset = ring->read_offset;
    int write_offset = ring->write_offset;
    int reed_size;
    if (read_offset == write_offset) {
        //没有什么要读的
        return 0;
    } else if (read_offset < write_offset) {
        //往后面读一点就可以
        int size = ring->write_offset - ring->read_offset;
        reed_size = buf_size > size ? size : buf_size;
        memcpy(buf, ring->data + read_offset, reed_size);
        ring->read_offset = ring->write_offset;
        return reed_size;
    } else {
        //需要从尾回到头开始读
        //读取后半部分
        int back_end = ring->size - read_offset;
        int mem_back_size = buf_size > back_end ? back_end : buf_size;
        memcpy(buf, ring->data + read_offset, mem_back_size);
        int buf_remain = buf_size - mem_back_size;
        if (buf_remain == 0) {
            ring->read_offset = ring->write_offset;
            return mem_back_size;
        }

        //读取前半部分
        int mem_front_size = buf_remain > write_offset ? write_offset : buf_remain;
        memcpy(buf + back_end, ring->data, mem_front_size);
        ring->read_offset = ring->write_offset;
        return mem_back_size + mem_front_size;
    }
}

//向环中写入数据
int send_ring(ring ring, const char *buf, int size) {
    //特殊情况判断
    if (size == 0) {
        return 0;
    }
    printf("send_ring: %s\n", buf + 2 * sizeof(int));
    int max_write = ring->size - 1;
    while (size > 0) {
        while (ring->write_offset != ring->read_offset) {
        }
        if (size <= max_write - ring->write_offset) {
            memcpy(ring->data + ring->write_offset, buf, size);
            printf("%s", ring->data + 2 * sizeof(int));
            ring->write_offset += size;
            if (ring->write_offset == ring->size) {
                ring->write_offset = 0;
            }
            printf("write OK\n");
            return 0;
        } else if (size <= max_write) {
            int back_end = max_write - ring->write_offset;
            int front_end = size - back_end;
            memcpy(ring->data + ring->write_offset, buf, back_end);
            memcpy(ring->data, buf + back_end, front_end);
            ring->write_offset = front_end;
            return 0;
        } else {
            int back_end = max_write - ring->write_offset;
            int front_end = max_write - back_end;
            memcpy(ring->data + ring->write_offset, buf, back_end);
            memcpy(ring->data, buf + back_end, front_end);
            ring->write_offset = front_end;
            size -= max_write;
            buf += max_write;
        }
    }
    return 0;
}
