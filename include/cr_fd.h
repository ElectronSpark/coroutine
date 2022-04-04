/* 负责管理文件描述符，让其适配异步操作 */
#ifndef __CR_FD_H__
#define __CR_FD_H__

#include <cr_types.h>

int cr_fd_init(void);
int cr_fd_register(int fd);
int cr_fd_unregister(int fd);
int cr_fd_set_unblock(int fd);
cr_fd_t *cr_fd_get(int fd);

#endif /* __CR_FD_H__ */