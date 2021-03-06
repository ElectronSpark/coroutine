#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include <coroutine.h>
#include <cr_socket.h>

static void __entry(void *param) {
    char buffer[64] = { 0 };
    int ret = -1;
    int sock = -1;

    if ((sock = cr_connect_tcp_server("127.0.0.1", 2334)) < 0) {
        printf("failed to connect to server\n");
        return;
    }

    sprintf(buffer, "this is %d ", (int)param);
    if (cr_send(sock, buffer, strlen(buffer), 0) <= 0) {
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
    if ((ret = cr_closesocket(sock)) != 0) {
        printf("failed to close sock: %d, ret: %d\n", sock, ret);
    }
}

int main(void)
{
    cr_init();

    for (int i = 0; i < 128; i++) {
        cr_task_create(__entry, (void *)i);
    }

    cr_wait_event_loop();

    return 0;
}