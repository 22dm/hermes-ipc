/*
 * 基于共享内存的 IPC
 *
 * 2019 byte-camp C02
 */

#include <sys/mman.h>
#include <fcntl.h>
#include <zconf.h>
#include <sys/un.h>
#include <string.h>

#define SHM_POOL_SIZE 1024 //描述符池大小
#define RING_SIZE 65536 //接收环大小（环部分）

typedef unsigned int socklen_t;

#define AF_UNIX 1
#define SOCK_STREAM 1

void *shm_base = NULL; //共享内存根地址
const char *shm_base_file = "/Users/nyako/shm_wow"; //共享内存文件

const int max_empty_times = 100; //连续读取失败超过此值即开始延迟
const int empty_close_times = 10000000; //连续读取失败超过此值即关闭连接

struct ring {
    int read_offset; //下一次从哪里开始读
    int write_offset; //下一次从哪里开始写
    int size; //环形队列大小，一次的最大写入量为 size - 1
    char data[RING_SIZE]; //环形队列
};

struct shm_socket_struct {
    int valid; //是否有效
    int parent_fd; //父描述符
    int child_fd[64]; //子描述符
    char addr_str[100]; //地址
    int max_connect; //最大连接数
    int send_fd; //发送描述符
    struct ring recv_ring_struct; //接收区域
};

typedef struct shm_socket_struct *shm_socket;

/*
 * 读取数据，只读一次
 */
int ring_recv(struct ring *ring, void *buf, int buf_size) {
    int read_size;
    int max_read;
    int ring_size = ring->size;
    if (ring->read_offset == ring->write_offset) {
        return 0;
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

/*
 * 向环中写入数据
 */
int ring_send(struct ring *ring, void *buf, int size) {
    int read_offset;
    int write_offset;
    int ring_size = ring->size;
    int max_write;
    int write_size;

    while (size > 0) {
        read_offset = ring->read_offset;
        write_offset = ring->write_offset;

        //判断是否可写
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

        //最多写入 4096 字节后，设置指针
        write_size = max_write < size ? max_write : size;
        write_size = write_size > 4096 ? 4096 : write_size;
        memcpy(ring->data + ring->write_offset, buf, write_size);

        //出界取模
        int after_write_offset = ring->write_offset + write_size;
        if (after_write_offset == ring_size) {
            ring->write_offset = 0;
        } else {
            ring->write_offset += write_size;
        }
        size -= write_size;
        buf += write_size;
    }
    return 0;
}

/*
 * 从共享内存中获取描述符
 */
shm_socket get_socket(int fd) {
    return (shm_socket) (shm_base + fd * sizeof(struct shm_socket_struct));
}

/*
 * 初始化描述符
 */
void shm_socket_init(int fd) {
    shm_socket shm_socket_obj = get_socket(fd);
    shm_socket_obj->valid = 1;
    shm_socket_obj->send_fd = -1;
    shm_socket_obj->parent_fd = -1;
    shm_socket_obj->max_connect = -1;
    shm_socket_obj->recv_ring_struct.size = RING_SIZE;
    shm_socket_obj->recv_ring_struct.read_offset = 0;
    shm_socket_obj->recv_ring_struct.write_offset = 0;
}

/*
 * 获取新的描述符
 */
int nsh_socket(int af, int socket, int type) {

    //是否需要读取根地址
    if (shm_base == NULL) {
        int metadata_size = sizeof(struct shm_socket_struct) * SHM_POOL_SIZE;
        int file_fd = shm_open(shm_base_file, O_CREAT | O_RDWR | O_EXCL, 0777);
        if (file_fd < 0) { // 对象已存在
            file_fd = shm_open(shm_base_file, O_RDWR, 0777);
        } else { // 新对象，设置大小
            ftruncate(file_fd, metadata_size);
        }
        shm_base = mmap(NULL, metadata_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0);
    }

    //获取最小的可用描述符
    for (int i = 0; i < SHM_POOL_SIZE; i++) {
        shm_socket shm_server_socket_obj = get_socket(i);
        int *val = &(shm_server_socket_obj->valid);
        //原子操作，解决并发问题
        if (__sync_bool_compare_and_swap (val, 0, 1)) {
            shm_server_socket_obj->valid = 1;
            return i;
        }
    }
    return -1;
}

/*
 * 绑定地址
 */
int nsh_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    //关闭已经绑定的 socket
    for (int i = 0; i < SHM_POOL_SIZE; i++) {
        shm_socket shm_server_socket_obj = get_socket(i);
        if (shm_server_socket_obj->valid == 0) {
            continue;
        }
        if (strcmp(shm_server_socket_obj->addr_str, ((struct sockaddr_un *) addr)->sun_path) == 0) {
            shm_server_socket_obj->valid = 0;
        }
    }

    //初始化描述符
    shm_socket_init(fd);
    shm_socket shm_socket_obj = get_socket(fd);
    strcpy(shm_socket_obj->addr_str, ((struct sockaddr_un *) addr)->sun_path);
    return 0;
}

/*
 * 接收链接，建立子描述符（阻塞）
 */
int nsh_accept(int fd, struct sockaddr *addr, socklen_t *addr_len) {
    //根据描述符读取对象，判断是否有效
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
            int child_send_fd = -1;
            do {
                ring_recv(&(shm_socket_obj->recv_ring_struct), (void *) &child_send_fd, sizeof(int));
                usleep(100);
            } while (child_send_fd == -1);
            //建立新的 fd 并记录
            int child_fd = nsh_socket(0, 0, 0);
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
    return -1;
}

/*
 * 确定最大连接数，初始化相关内容
 */
int nsh_listen(int fd, int max_connect) {
    //根据描述符读取对象，判断是否有效
    shm_socket shm_socket_obj = get_socket(fd);
    if (shm_socket_obj->valid == 0) {
        return -1;
    }

    //填入最大连接数，子句柄列表初始化为 -1
    shm_socket_obj->max_connect = max_connect;
    int *child_fds = shm_socket_obj->child_fd;
    for (int i = 0; i < max_connect; i++) {
        child_fds[i] = -1;
    }

    return 0;
}

/*
 * 创建接收内存，并与 Server 端交换内存
 */

int nsh_connect(int fd, struct sockaddr *addr, socklen_t addr_len) {
    //初始化描述符
    shm_socket_init(fd);
    shm_socket shm_socket_obj = get_socket(fd);

    //寻找 addr 对应的 Server
    for (int i = 0; i < SHM_POOL_SIZE; i++) {
        shm_socket shm_server_socket_obj = get_socket(i);
        if (shm_server_socket_obj->valid == 0) {
            continue;
        }
        if (strcmp(shm_server_socket_obj->addr_str, ((struct sockaddr_un *) addr)->sun_path) == 0) {
            ring_send(&(shm_server_socket_obj->recv_ring_struct), (void *) &fd, sizeof(int));
            //while (shm_socket_obj->send_fd == -1);
            return 0;
        }
    }
    return -1;
}

/*
 * 关闭描述符
 */
int nsh_close(int fd) {
    //根据描述符读取对象，判断是否有效
    shm_socket shm_socket_obj = get_socket(fd);
    if (shm_socket_obj->valid == 0) {
        return -1;
    }

    //如果是 Server 端
    if (shm_socket_obj->parent_fd == -1 && shm_socket_obj->max_connect > 0) {
        for (int i = 0; i < shm_socket_obj->max_connect; i++) {
            int child_fd = shm_socket_obj->child_fd[i];
            nsh_close(child_fd);
        }
    }

    //如果已经建立连接，关掉对方
    if (shm_socket_obj->send_fd != -1) {
        get_socket(shm_socket_obj->send_fd)->valid = 0;
    }

    //关闭自己
    shm_socket_obj->valid = 0;

    return 0;
}

/*
 * 接收消息，阻塞
 */
int nsh_recv(int fd, void *buf, size_t buf_size, int type) {
    //根据描述符读取对象，判断是否有效
    shm_socket shm_socket_obj = get_socket(fd);
    if (shm_socket_obj->valid == 0) {
        return -1;
    }

    int recv_size = 0, total_recv_size = 0;
    //记录读取失败的次数
    int empty_times = 0;
    do {
        recv_size = ring_recv(&(shm_socket_obj->recv_ring_struct), buf + total_recv_size, buf_size - total_recv_size);
        total_recv_size += recv_size;
        if (total_recv_size == 0) {
            empty_times++;
            //判断是否开始退出
            if (empty_times >= empty_close_times) {
                close(fd);
                return -1;
            }
            //判断是否阻塞
            if (empty_times >= max_empty_times) {
                usleep(1);
            }
        }
    } while (total_recv_size == 0 || (recv_size > 0 && total_recv_size < buf_size));
    return total_recv_size;
}

/*
 * 发送消息（阻塞）
 */
int nsh_send(int fd, void *data, size_t size, int type) {
    //根据描述符读取对象，判断是否有效
    shm_socket shm_socket_obj = get_socket(fd);
    if (shm_socket_obj->valid == 0) {
        return -1;
    }

    ring_send(&(get_socket(shm_socket_obj->send_fd)->recv_ring_struct), data, size);
    return 0;
}
