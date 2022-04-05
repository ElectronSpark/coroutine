#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <coroutine.h>
#include <cr_fd.h>
#include <cr_epoll.h>


static int __epfd = -1;

#define __check_epfd_valid()    (__epfd >= 0)


/* 协程调度器所调用的 idle 函数 */
static void __cr_epoll_idle_handler(void *param)
{
    int timeout = 0;
    if (cr_is_waitqueue_empty(cr_global()->ready_queue)) {
        timeout = -1;
    }

    cr_epoll_wait(timeout);
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
    struct epoll_event epevent = {
        .events = events,
        .data.fd = fd
    };

    if (!__check_epfd_valid()) {
        return -CR_ERR_FAIL;
    }

    return epoll_ctl(__epfd, EPOLL_CTL_ADD, fd, &epevent);
}

/* 删除对一个文件的监听 */
int cr_epoll_del(int fd)
{
    if (!__check_epfd_valid()) {
        return -CR_ERR_FAIL;
    }

    return epoll_ctl(__epfd, EPOLL_CTL_DEL, fd, NULL);
}

/* 修改对文件进行监听的事件 */
int cr_epoll_mod(int fd, int events)
{
    struct epoll_event epevent = {
        .events = events,
        .data.fd = fd
    };

    if (!__check_epfd_valid()) {
        return -CR_ERR_FAIL;
    }

    return epoll_ctl(__epfd, EPOLL_CTL_MOD, fd, &epevent);
}

/* 等待 epoll 返回事件，并根据这些事件唤醒相应的协程 */
int cr_epoll_wait(int timeout)
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
        fditem->events = events[i].events;
        if (events[i].events & (EPOLLERR | EPOLLHUP)) {
            cr_waitqueue_notify_all(fditem->read_queue, -CR_ERR_CLOSE);
            cr_waitqueue_notify_all(fditem->write_queue, -CR_ERR_CLOSE);
            cr_waitqueue_notify_all(fditem->wait_queue, -CR_ERR_CLOSE);
        } else {
            cr_waitqueue_notify(fditem->wait_queue, CR_ERR_OK);
            if (events[i].events & EPOLLIN) {
                cr_waitqueue_notify(fditem->read_queue, CR_ERR_OK);
            }
            if (events[i].events & EPOLLOUT) {
                cr_waitqueue_notify(fditem->write_queue, CR_ERR_OK);
            }
        }
    }

    return CR_ERR_OK;
}
