#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>

#include <coroutine.h>
#include <cr_fd.h>
#include <cr_epoll.h>


static __epfd = -1;

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

/*  */
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

/*  */
int cr_epoll_del(int fd)
{
    if (!__check_epfd_valid()) {
        return -CR_ERR_FAIL;
    }

    return epoll_ctl(__epfd, EPOLL_CTL_DEL, fd, NULL);
}

/*  */
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

/*  */
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
        fditem = cr_fd_get(events->data.fd);
        if (fditem == NULL) {
            continue;
        }
        fditem->events = events;
        cr_sem_post(fditem->wait_sem);
        if (events->events & (EPOLLERR | EPOLLHUP)) {
            cr_sem_close(fditem->read_sem);
            cr_sem_close(fditem->write_sem);
            cr_sem_close(fditem->wait_sem);
        } else {
            if (events->events & EPOLLIN) {
                cr_sem_post(fditem->read_sem);
            }
            if (events->events & EPOLLOUT) {
                cr_sem_post(fditem->write_sem);
            }
        }
    }

    return CR_ERR_OK;
}