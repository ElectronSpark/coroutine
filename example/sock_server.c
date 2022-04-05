#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>

#include <coroutine.h>
#include <cr_socket.h>

static void __entry(void *param) {
    struct sockaddr_in srv_addr = { 0 };
    struct sockaddr_in clnt_addr = { 0 };
    socklen_t socklen = sizeof(struct sockaddr_in);
    socklen_t clntlen = sizeof(struct sockaddr_in);
    char buffer[64] = { 0 };
    int ret = -1;
    int sock = -1;
    int clnt_sock = -1;
    int reuse = 1;

    sock = cr_socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("failed to create sock. fd: %d\n", sock);
        return;
    }
    ret = setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, 
                     (char *)&reuse, sizeof(reuse));
    if (ret != 0) {
        printf("failed to set SO_REUSEPORT");
    }
    printf("socket created. fd: %d\n", sock);

    if (cr_make_sockaddr(&srv_addr, &socklen, "", 2334) != CR_ERR_OK) {
        printf("failed to make sockaddr\n");
        goto __entry_exit;
    }
    if (bind(sock, &srv_addr, socklen) != CR_ERR_OK) {
        printf("failed to bind, errno:%d\n", errno);
        goto __entry_exit;
    }
    if (listen(sock, 5) != CR_ERR_OK) {
        printf("failed to listen\n");
        goto __entry_exit;
    }

    if ((clnt_sock = cr_accept(sock, &clnt_addr, &clntlen)) <= 0) {
        printf("failed to accept\n");
        goto __entry_exit;
    }
    if ((ret = cr_recv(clnt_sock, buffer, 64, 0)) <= 0) {
        printf("failed to receive data\n");
        goto __entry_exit_clnt;
    }
    buffer[ret] = '\0';
    strcat(buffer, "world!");

    if (cr_send(clnt_sock, buffer, strlen(buffer), 0) <= 0) {
        printf("failed to send string\n");
        goto __entry_exit_clnt;
    }

    printf("send: \"%s\"\n", buffer);

__entry_exit_clnt:
    if ((ret = cr_closesocket(clnt_sock)) != 0) {
        printf("failed to close clnt_sock: %d, ret: %d\n", clnt_sock, ret);
    }
__entry_exit:
    if ((ret = cr_closesocket(sock)) != 0) {
        printf("failed to close sock: %d, ret: %d\n", sock, ret);
    }
}

int main(void)
{
    cr_init();

    cr_task_create(__entry, NULL);

    cr_wait_event_loop();

    return 0;
}