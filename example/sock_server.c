#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>

#include <coroutine.h>
#include <cr_socket.h>

static void __handler(void *param)
{
    int sock = (int)param;
    int ret = -1;
    char buffer[64] = { 0 };

    printf("fd(%d): __handler entered.\n", sock);

    if ((ret = cr_recv(sock, buffer, 64, 0)) <= 0) {
        printf("fd(%d): failed to receive data\n", sock);
        goto __handler_exit;
    }
    buffer[ret] = '\0';
    printf("fd(%d) receive: \"%s\"\n", sock, buffer);

    strcat(buffer, "-- ok");
    if (cr_send(sock, buffer, strlen(buffer), 0) <= 0) {
        printf("fd(%d): failed to send string\n", sock);
        goto __handler_exit;
    }
    printf("fd(%d) send: \"%s\"\n", sock, buffer);

__handler_exit:
    if ((ret = cr_closesocket(sock)) != 0) {
        printf("__handler: failed to close sock: %d, ret: %d\n", sock, ret);
    }
}

static void __entry(void *param) {
    struct sockaddr_in clnt_addr = { 0 };
    socklen_t clntlen = sizeof(struct sockaddr_in);
    cr_task_t *task = NULL;
    int ret = -1;
    int sock = -1;
    int clnt_sock = -1;
    int reuse = 1;

    sock = cr_create_tcp_server("", 2334);
    if (sock < 0) {
        printf("failed to create server, errno:%d\n", errno);
        return;
    }
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char *)&reuse, sizeof(reuse));

    if (listen(sock, 5) != CR_ERR_OK) {
        printf("failed to listen\n");
        goto __entry_exit;
    }

    while (1) {
        clnt_sock = cr_accept(sock, (struct sockaddr *)&clnt_addr, &clntlen);
        if (clnt_sock <= 0) {
            printf("failed to accept\n");
            continue;
        }
        task = cr_task_create(__handler, (void *)clnt_sock);
        if (task == NULL) {
            printf("failed to create a handler!\n");
            if ((ret = cr_closesocket(clnt_sock)) != 0) {
                printf("failed to close clnt_sock: %d, ret: %d\n", clnt_sock, ret);
            }
            continue;
        }
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