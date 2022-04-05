/* 配合协程的异步操作对 socket 的封装 */
#ifndef __CR_SOCKET_H__
#define __CR_SOCKET_H__

#include <cr_types.h>
#include <sys/socket.h>


int cr_make_sockaddr(struct sockaddr_in *addr, socklen_t *addrlen,
                     const char *host, const unsigned short port);
int cr_create_tcp_server(const char *host, const unsigned short port);
int cr_connect_tcp_server(const char *host, const unsigned short port);

int cr_socket(int domain, int type, int protocol);
int cr_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int cr_closesocket(int sockfd);

ssize_t cr_recv(int socket, void *buffer, size_t length, int flags );
ssize_t cr_send(int socket, const void *buffer, size_t length, int flags);

ssize_t cr_recvfrom(int socket, void *buffer, size_t length,
	                int flags, struct sockaddr *address,
					socklen_t *address_len);
ssize_t cr_sendto(int socket, const void *message, size_t length,
	              int flags, const struct sockaddr *dest_addr,
				  socklen_t dest_len);

int cr_connect(int sockfd, const struct sockaddr *address,
               socklen_t address_len);


#endif /* __CR_SOCKET_H__ */
