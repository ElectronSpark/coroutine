#include <coroutine.h>
#include <stdio.h>

#define CR_CHANNELS_NUM     100000
#define CR_CH_GROUP_SIZE    16
#define CR_CH_BUFFER_ITEMS  3


struct my_channel {
    cr_channel_t    ch[1];
    void    *buffer[CR_CH_BUFFER_ITEMS];
};

int send_count = 0;
int receive_count = 0;
int close_count = 0;
int exit_count = 0;

static void __receiver_entry(void *param) {
    cr_channel_t *ch = param;
    void *data = (void *)0xabababab;

    while (cr_channel_recv(ch, &data) == 0) {
        if (data == NULL) {
            receive_count += 1;
        }
    }

    exit_count += 1;
    cr_task_exit();
    exit_count -= 1;
}

static void __sender_entry(void *param) {
    cr_channel_t *ch = param;
    int i = 0;

    for (i = 0; i < CR_CH_GROUP_SIZE * 2; i++) {
        if (cr_channel_send(ch, NULL) == 0) {
            send_count += 1;
        }
    }

    cr_channel_flush(ch, NULL);

    if (cr_channel_close(ch) == 0) {
        close_count++;
    }

    exit_count += 1;
    cr_task_exit();
    exit_count -= 1;
}

int main(void)
{
    static struct my_channel ch[CR_CHANNELS_NUM];
    cr_init();

    for (long i = 0; i < CR_CHANNELS_NUM; i++) {
        cr_channel_init(ch[i].ch, CR_CH_BUFFER_ITEMS);
        cr_task_create(__sender_entry, (void *)ch[i].ch);
        for (int j = 1; j < CR_CH_GROUP_SIZE; j++) {
            cr_task_create(__receiver_entry, (void *)ch[i].ch);
        }
    }

    cr_wait_event_loop();

    printf("channels: %d, group_size: %d\n",
           CR_CHANNELS_NUM,
           CR_CH_GROUP_SIZE);
    printf("send_count:%d, receive_count:%d\n", send_count, receive_count);
    printf("close_count:%d, exit_count:%d\n", close_count, exit_count);

    return 0;
}
