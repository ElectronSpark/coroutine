/* 借助 epoll 进行协程异步操作的的接口 */
#ifndef __CR_EPOLL_H__
#define __CR_EPOLL_H__

#include <cr_types.h>


int cr_epoll_init(void);
int cr_epoll_close(void);
int cr_epoll_add(int fd, int events);
int cr_epoll_del(int fd);
int cr_epoll_mod(int fd, int events);
int cr_epoll_waitevent(int fd, int events, int *retevents);
int cr_epoll_notify(int timeout);

#endif /* __CR_EPOLL_H__ */
