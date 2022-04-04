#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <coroutine.h>
#include <cr_pool.h>
#include <cr_fd.h>


/* 储存所有可以异步处理的文件描述符的表 */
static int      __max_fds = -1;
static cr_fd_t  **__fdstab = NULL;

/* 分配 __fdstab 内元素的内存池 */
static CR_POOL_DECLARE(__fditem_pool, sizeof(cr_fd_t),    \
                       CR_DEFAULT_POOL_SIZE,    \
                       CR_DEFAULT_POOL_SIZE / sizeof(cr_fd_t));


/* 判断一个 fd 是否在 */
#define __is_fd_valid(__fd)     ((__fd) >= 0 && (__fd) < __max_fds)

/* 在内存池中分配和释放 cr_fd_t */
#define __fditem_alloc()        cr_pool_block_alloc(&__fditem_pool)
#define __fditem_free(__ptr)    cr_pool_block_free(&__fditem_pool, (__ptr))


/* 初始化一个 cr_fd_t 结构体 */
static inline int __fditem_init(cr_fd_t *fditem, int fd)
{
    int ret = CR_ERR_OK;

    if ((ret = cr_sem_init(fditem->wait_sem, 1, 1)) != CR_ERR_OK) {
        return ret;
    }
    if ((ret = cr_sem_init(fditem->read_sem, 1, 1)) != CR_ERR_OK) {
        return ret;
    }
    if ((ret = cr_sem_init(fditem->write_sem, 1, 1)) != CR_ERR_OK) {
        return ret;
    }
    fditem->fd = fd;
    fditem->events = 0;

    return CR_ERR_OK;
}

/* 更新文件描述符数量的限制 */
static inline int __fdstab_limit_update(void)
{
    struct rlimit limit = { 0 };
    int final_limit = -1;
    
    if (getrlimit(RLIMIT_NOFILE, &limit) != 0) {
        return -CR_ERR_FAIL;
    }

    final_limit = 1 << 20; /* 限制打开的文件描述符的个数为大约一百万 */
    if (limit.rlim_max < final_limit) {
        final_limit = limit.rlim_max;
    }
    limit.rlim_cur = final_limit;
    if (setrlimit(RLIMIT_NOFILE, &limit) != 0) {
        return -CR_ERR_FAIL;
    }

    __max_fds = final_limit;
    return CR_ERR_OK;
}

/* 检查一个 fd 是否已经适应的协程框架的异步操作。返回布尔值 */
static inline int __check_fd(int fd)
{
    if (!__is_fd_valid(fd)) {
        return 0;
    }
    if (!__fdstab || !__fdstab[fd]) {
        return 0;
    }
    return 1;
}

/* 创建并初始化 __fdstab */
int cr_fd_init(void)
{
    int ret = CR_ERR_OK;
    cr_fd_t **tmp_tab = NULL;

    if (__fdstab != NULL) {
        return -CR_ERR_FAIL;
    }

    if ((ret = __fdstab_limit_update()) != CR_ERR_OK) {
        return ret;
    }

    tmp_tab = calloc(__max_fds, sizeof(cr_fd_t *));
    if (tmp_tab == NULL) {
        __max_fds = -1;
        return -CR_ERR_FAIL;
    }

    memset(tmp_tab, 0, __max_fds * sizeof(cr_fd_t *));
    __fdstab = tmp_tab;
    return CR_ERR_OK;
}

/* 尝试将 fd 加入 __fdstab 中，使得 fd 能适应协程框架的操作。 */
int cr_fd_register(int fd)
{
    cr_fd_t *fditem = NULL;

    if (!__is_fd_valid(fd)) {
        return -CR_ERR_INVAL;
    }
    if (!__fdstab || !__fdstab[fd]) {
        return -CR_ERR_FAIL;
    }

    if ((fditem = __fditem_alloc()) == NULL) {
        return -CR_ERR_FAIL;
    }
    if (__fditem_init(fditem, fd) != CR_ERR_OK) {
        __fditem_free(fditem);
        return -CR_ERR_FAIL;
    }
    __fdstab[fd] = fditem;
    return CR_ERR_OK;
}

/* 将 fd 从 __fdstab 表中移除，并唤醒正在等待 fd 的协程 */
int cr_fd_unregister(int fd)
{
    if (!__check_fd(fd)) {
        return -CR_ERR_INVAL;
    }

    cr_sem_close(__fdstab[fd]->wait_sem);
    cr_sem_close(__fdstab[fd]->read_sem);
    cr_sem_close(__fdstab[fd]->write_sem);
    __fditem_free(__fdstab[fd]);
    __fdstab[fd] = NULL;
    return CR_ERR_OK;
}

/* 将文件描述符设置为非阻塞，以适应协程的异步操作 */
int cr_fd_set_unblock(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    flag |= O_NONBLOCK | O_NDELAY;
    return fcntl(fd, F_SETFL, flag);
}

/* 获得文件描述符的 cr_fd_t 结构体 */
cr_fd_t *cr_fd_get(int fd)
{
    if (!__check_fd(fd)) {
        return NULL;
    }
    if (__fdstab[fd]->fd != fd) {
        return NULL;
    }
    return __fdstab[fd];
}
