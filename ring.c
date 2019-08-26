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
    printf("1111: %d\n", size);
    int fd = shm_open(file, O_CREAT | O_RDWR | O_EXCL, 0777);
    if (fd < 0) {
        perror("对象已存在");
        // 对象已存在
        printf("对象已存在\n");

        fd = shm_open(file, O_RDWR, 0777);

    } else {
        // 新对象，设置大小
        printf("1111: %d\n", size);

        int a = ftruncate(fd, size + sizeof(struct ring_struct));

    }
    printf("1111\n");


    perror("aaaaa");

    ring ring = mmap(NULL, size + sizeof(struct ring_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    ring->size = size;
    ring->read_offset = 0;
    ring->write_offset = 0;
    return ring;
}

//读取数据，只读一次
int recv_ring(ring ring, char *buf, int buf_size) {
#ifdef DEBUG
    printf("尝试读取\n");
#endif
    int reed_size;
    while (ring->read_offset == ring->write_offset) {
        //没有什么要读的
    }
    int read_offset = ring->read_offset;
    int write_offset = ring->write_offset;
#ifdef DEBUG
    printf("读-写入偏移: %d\n", ring->read_offset);
    printf("读-读取偏移: %d\n", ring->write_offset);
#endif
    if (read_offset < write_offset) {
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
    int max_write = ring->size - 1;
    while (size > 0) {
        while (ring->write_offset != ring->read_offset) {
        }
#ifdef DEBUG
        printf("最大写入数据: %d\n", max_write);
        printf("写入数据: %s\n", buf + 2 * sizeof(int));
        printf("写入数据大小: %d\n", size);
#endif
        if (size <= max_write - ring->write_offset) {
#ifdef DEBUG
            printf("实际写入数据（单向）: %s\n", buf + 2 * sizeof(int));
            printf("实际写入数据大小: %d\n", size);
#endif
            memcpy(ring->data + ring->write_offset, buf, size);
            ring->write_offset += size;
            if (ring->write_offset == ring->size) {
                ring->write_offset = 0;
            }
#ifdef DEBUG
            printf("写入偏移: %d\n", ring->read_offset);
            printf("读取偏移: %d\n", ring->write_offset);
#endif
            return 0;
        } else if (size <= max_write) {
#ifdef DEBUG
            printf("实际写入数据（双向）: %s\n", buf + 2 * sizeof(int));
            printf("实际写入数据大小: %d\n", size);
#endif
            int back_end = ring->size - ring->write_offset;
            int front_end = size - back_end;
#ifdef DEBUG
            printf("后部大小: %d\n", back_end);
            printf("前部大小: %d\n", front_end);
#endif
            memcpy(ring->data + ring->write_offset, buf, back_end);
            memcpy(ring->data, buf + back_end, front_end);
            ring->write_offset = front_end;
#ifdef DEBUG
            printf("写入偏移: %d\n", ring->read_offset);
            printf("读取偏移: %d\n", ring->write_offset);
#endif
            return 0;
        } else {
#ifdef DEBUG
            printf("实际写入数据（满）: %s\n", buf + 2 * sizeof(int));
            printf("实际写入数据大小: %d\n", max_write);
#endif
            int back_end = max_write - ring->write_offset;
            int front_end = max_write - back_end;
            memcpy(ring->data + ring->write_offset, buf, back_end);
            if (front_end == 0) {
                ring->write_offset = ring->size - 1;
            } else {
                memcpy(ring->data, buf + back_end, front_end);
                ring->write_offset = front_end - 1;
            }
#ifdef DEBUG
            printf("写入偏移: %d\n", ring->read_offset);
            printf("读取偏移: %d\n", ring->write_offset);
#endif
            size -= max_write;
            buf += max_write;
        }
    }
    return 0;
}

void delete_ring(ring ring){

}
