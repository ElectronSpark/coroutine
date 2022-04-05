#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include <coroutine.h>
#include <cr_socket.h>

static void __entry(void *param) {
    struct sockaddr_in srv_addr = { 0 };
    socklen_t socklen = sizeof(struct sockaddr_in);
    char buffer[64] = { 0 };
    const char *send_str = "hello ";
    int ret = -1;
    int sock = -1;

    sock = cr_socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("failed to create sock. fd: %d\n", sock);
        return;
    }
    printf("socket created. fd: %d\n", sock);

    if (cr_make_sockaddr(&srv_addr, &socklen, "0.0.0.0", 2334) != CR_ERR_OK) {
        printf("failed to make sockaddr\n");
        goto __entry_exit;
    }
    if (cr_connect(sock, &srv_addr, socklen) != CR_ERR_OK) {
        printf("failed to connect\n");
        goto __entry_exit;
    }

    if (cr_send(sock, send_str, strlen(send_str), 0) <= 0) {
        printf("failed to send string\n");
        goto __entry_exit;
    }
    if ((ret = cr_recv(sock, buffer, 64, 0)) <= 0) {
        printf("failed to receive data\n");
        goto __entry_exit;
    }
    buffer[ret] = '\0';

    printf("received: \"%s\"\n", buffer);

__entry_exit:
    cr_closesocket(sock);
}

int main(void)
{
    cr_init();

    cr_task_create(__entry, NULL);

    cr_wait_event_loop();

    return 0;
}