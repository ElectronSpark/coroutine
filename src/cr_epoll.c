#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <coroutine.h>
#include <cr_fd.h>
#include <cr_epoll.h>


static int __epfd = -1;
/* 检查当前 __epfd 的有效性 */
#define __check_epfd_valid()    (__epfd >= 0)

/* 对 __epfd 的操作 */
#define __epoll_ctl(__fd, __cmd, __events)  ({  \
    struct epoll_event __evt = {    \
        .events = (__events),   \
        .data.fd = (__fd)   \
    };  \
    epoll_ctl(__epfd, __cmd, __fd, &__evt); \
})
/* 向 epoll 中加入 fd */
#define __epoll_ctl_add(__fd, __events) \
    __epoll_ctl(__fd, EPOLL_CTL_ADD, __events)
/* 修改 epoll 中对 fd 进行监听的事件 */
#define __epoll_ctl_mod(__fd, __events) \
    __epoll_ctl(__fd, EPOLL_CTL_MOD, __events)
/* 从 epoll 中删除 fd */
#define __epoll_ctl_del(__fd)   \
    __epoll_ctl(__fd, EPOLL_CTL_DEL, 0)


/* 协程调度器所调用的 idle 函数 */
static void __cr_epoll_idle_handler(void *param)
{
    int timeout = 0;
    if (cr_is_waitqueue_empty(cr_global()->ready_queue)) {
        timeout = -1;
    }

    cr_epoll_notify(timeout);
}

/* 创建全局的 epoll 文件描述符 */
int cr_epoll_init(void)
{
    int fd = -1;

    if (__check_epfd_valid()) {
        return -CR_ERR_INVAL;
    }
    if ((fd = epoll_create1(0)) < 0) {
        return -CR_ERR_FAIL;
    }
    __epfd = fd;

    if (cr_set_idle_handler(__cr_epoll_idle_handler) == 0) {
        return CR_ERR_OK;
    } else {
        return -CR_ERR_FAIL;
    }
}

/* 关闭全局的 epoll 文件描述符 */
int cr_epoll_close(void)
{
    if (!__check_epfd_valid()) {
        return -CR_ERR_FAIL;
    }
    close(__epfd);
    __epfd = -1;

    return CR_ERR_OK;
}

/* 添加对一个文件的监听 */
int cr_epoll_add(int fd, int events)
{
    if (!__check_epfd_valid()) {
        return -CR_ERR_FAIL;
    }
    return __epoll_ctl_add(fd, events);
}

/* 删除对一个文件的监听 */
int cr_epoll_del(int fd)
{
    if (!__check_epfd_valid()) {
        return -CR_ERR_FAIL;
    }
    return __epoll_ctl_del(fd);
}

/* 修改对文件进行监听的事件 */
int cr_epoll_mod(int fd, int events)
{
    if (!__check_epfd_valid()) {
        return -CR_ERR_FAIL;
    }
    return __epoll_ctl_mod(fd, events);
}

/**/
int cr_epoll_waitevent(int fd, int events, int *retevents)
{
    int ret = CR_ERR_OK;
    cr_fd_t *fditem = NULL;
    
    if ((fditem = cr_fd_get(fd, 1)) == NULL) {
        return -CR_ERR_FAIL;
    }
    if (cr_is_fd_busy(fditem)) {
        ret = __epoll_ctl_mod(fd, events);
    } else {
        ret = __epoll_ctl_add(fd, events);
    }
    if (ret != 0) {
        return -CR_ERR_FAIL;
    }

    return cr_fd_wait(fditem, retevents);
}

/* 等待 epoll 返回事件，并根据这些事件唤醒相应的协程 */
int cr_epoll_notify(int timeout)
{
    static struct epoll_event events[512] = { 0 };
    cr_fd_t *fditem = NULL;
    int nfds = 0;
    int fd = -1;

    if (!__check_epfd_valid()) {
        return -CR_ERR_FAIL;
    }

    nfds = epoll_wait(__epfd, events,
                      sizeof(events) / sizeof(struct epoll_event),
                      timeout);
    if (nfds < 0) {
        return -CR_ERR_FAIL;
    }

    for (int i = 0; i < nfds; i++) {
        fditem = cr_fd_get(events[i].data.fd, 0);
        if (fditem == NULL) {
            continue;
        }
        cr_fd_notify(fditem, events[i].events);
        if (!cr_is_fd_busy(fditem)) {
            __epoll_ctl_del(events[i].data.fd);
        }
    }

    return CR_ERR_OK;
}
