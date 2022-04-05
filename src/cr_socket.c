#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <coroutine.h>
#include <cr_socket.h>
#include <cr_epoll.h>
#include <cr_fd.h>


int cr_make_sockaddr(struct sockaddr_in *addr, socklen_t *addrlen,
                     const char *host, const unsigned short port)
{
    int ip = 0;

    if (!addr || !addrlen) {
        return -CR_ERR_INVAL;
    }

    if (!host || '\0' == host[0]) {
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        addr->sin_addr.s_addr = inet_addr(host);
    }
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    *addrlen = sizeof(struct sockaddr_in);

    return CR_ERR_OK;
}

int cr_create_tcp_server(const char *host, const unsigned short port)
{

}

int cr_socket(int domain, int type, int protocol)
{
    int ret = 0;
    int ret_fd = -1;
    
    ret_fd = socket(domain, type, protocol);
    if (ret_fd < 0) {
        return ret_fd;
    }
    if ((ret = cr_fd_set_unblock(ret_fd)) != CR_ERR_OK) {
        close(ret_fd);
        return ret;
    }
    if ((ret = cr_fd_register(ret_fd)) != CR_ERR_OK) {
        close(ret_fd);
        return ret;
    }
    return ret_fd;
}

int cr_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int ret = 0;
    int ret_fd = 0;
    cr_fd_t *fditem = NULL;

    if (!addr || !addrlen) {
        return -CR_ERR_INVAL;
    }

    if ((fditem = cr_fd_get(sockfd, 1)) == NULL) {
        return -CR_ERR_FAIL;
    }

    for (;;) {
        ret_fd = accept(sockfd, addr, addrlen);
        if (ret_fd < 0) {
            if (errno == EAGAIN) {
                ret = cr_epoll_add(sockfd, EPOLLIN|EPOLLOUT|EPOLLHUP);
                if (ret != 0) {
                    return -CR_ERR_FAIL;
                }
                if ((ret = cr_await(fditem->read_queue)) != CR_ERR_OK) {
                    return ret;
                }
                continue;
            }
        }

        if ((ret = cr_fd_register(ret_fd)) != CR_ERR_OK) {
            close(ret_fd);
            return ret;
        }
        return ret_fd;
    }
}

int cr_closesocket(int sockfd)
{
    int ret = 0;
    cr_fd_t *fditem = NULL;

    if ((fditem = cr_fd_get(sockfd, 1)) == NULL) {
        return -CR_ERR_FAIL;
    }
    if ((ret = cr_fd_freeze(fditem)) != CR_ERR_OK) {
        return ret;
    }

    ret = cr_waitqueue_notify_all(fditem->read_queue, -CR_ERR_CLOSE);
    while (ret == CR_ERR_OK) {
        if (!cr_is_waitqueue_empty(fditem->read_queue)) {
            ret = cr_await(fditem->wait_queue);
            continue;
        }
        if (!cr_is_waitqueue_empty(fditem->write_queue)) {
            ret = cr_await(fditem->wait_queue);
            continue;
        }
        break;
    }

    if (ret != CR_ERR_OK) {
        cr_waitqueue_notify_all(fditem->read_queue, -CR_ERR_CLOSE);
        cr_waitqueue_notify_all(fditem->write_queue, -CR_ERR_CLOSE);
    }

    return cr_fd_unregister(sockfd);
}

ssize_t cr_recv(int socket, void *buffer, size_t length, int flags )
{
    ssize_t ret = 0;
    cr_fd_t *fditem = NULL;

    if ((fditem = cr_fd_get(socket, 1)) == NULL) {
        return -CR_ERR_FAIL;
    }
    for (;;) {
        ret = recv(socket, buffer, length, flags);
        if (ret < 0) {
            if (errno != EAGAIN) {
                return -CR_ERR_FAIL;
            }
            if (cr_epoll_add(socket, EPOLLIN) != 0) {
                return -CR_ERR_FAIL;
            }
            if ((ret = cr_await(fditem->read_queue)) == CR_ERR_OK) {
                continue;
            }
        }
        return ret;
    }
}

ssize_t cr_send(int socket, const void *buffer, size_t length, int flags)
{
    ssize_t ret = 0;
    cr_fd_t *fditem = NULL;

    if ((fditem = cr_fd_get(socket, 1)) == NULL) {
        return -CR_ERR_FAIL;
    }
    for (;;) {
        ret = send(socket, buffer, length, flags);
        if (ret < 0) {
            if (errno != EAGAIN) {
                return -CR_ERR_FAIL;
            }
            if (cr_epoll_add(socket, EPOLLOUT) != 0) {
                return -CR_ERR_FAIL;
            }
            if ((ret = cr_await(fditem->write_queue)) == CR_ERR_OK) {
                continue;
            }
        }
        return ret;
    }
}

ssize_t cr_recvfrom(int socket, void *buffer, size_t length,
	                int flags, struct sockaddr *address,
					socklen_t *address_len)
{
    ssize_t ret = 0;
    cr_fd_t *fditem = NULL;

    if ((fditem = cr_fd_get(socket, 1)) == NULL) {
        return -CR_ERR_FAIL;
    }
    for (;;) {
        ret = recvfrom(socket, buffer, length, flags, address, address_len);
        if (ret < 0) {
            if (errno != EAGAIN) {
                return -CR_ERR_FAIL;
            }
            ret = cr_epoll_add(socket, EPOLLIN);
            if (ret != 0) {
                return -CR_ERR_FAIL;
            }
            if ((ret = cr_await(fditem->read_queue)) != CR_ERR_OK) {
                return ret;
            }
            continue;
        }
        return ret;
    }
}

ssize_t cr_sendto(int socket, const void *message, size_t length,
	              int flags, const struct sockaddr *dest_addr,
				  socklen_t dest_len)
{
    ssize_t ret = 0;
    cr_fd_t *fditem = NULL;

    if ((fditem = cr_fd_get(socket, 1)) == NULL) {
        return -CR_ERR_FAIL;
    }
    for (;;) {
        ret = sendto(socket, message, length, flags, dest_addr, dest_len);
        if (ret < 0) {
            if (errno != EAGAIN) {
                return -CR_ERR_FAIL;
            }
            ret = cr_epoll_add(socket, EPOLLOUT);
            if (ret != 0) {
                return -CR_ERR_FAIL;
            }
            if ((ret = cr_await(fditem->write_queue)) != CR_ERR_OK) {
                return ret;
            }
            continue;
        }
        return ret;
    }
}

int cr_connect(int sockfd, const struct sockaddr *address,
               socklen_t address_len)
{
    int ret = 0;
    cr_fd_t *fditem = NULL;

    if (!address) {
        return -CR_ERR_INVAL;
    }

    if ((fditem = cr_fd_get(sockfd, 1)) == NULL) {
        return -CR_ERR_FAIL;
    }

    for (;;) {
        ret = connect(sockfd, address, address_len);
        if (ret < 0) {
            if (errno == EINPROGRESS) {
                ret = cr_epoll_add(sockfd, EPOLLIN|EPOLLOUT|EPOLLHUP);
                if (ret != 0) {
                    return -CR_ERR_FAIL;
                }
                if ((ret = cr_await(fditem->write_queue)) != CR_ERR_OK) {
                    return ret;
                }
                continue;
            } else {
                return -CR_ERR_FAIL;
            }
        }
        return ret;
    }
}
