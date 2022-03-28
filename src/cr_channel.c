#include <string.h>
#include <malloc.h>

#include <coroutine.h>
#include <cr_waitable.h>


#define __ch_flag_set_opened(ch)    do { (ch)->flag.opened = 1; } while (0)
#define __ch_flag_unset_opened(ch)  do { (ch)->flag.opened = 0; } while (0)

#define __ch_buffer_size(ch)    ((ch)->buffer_size)
#define __ch_buffer_empty(ch)   (cr_chan_count(ch) == 0)
#define __ch_buffer_full(ch)    (cr_chan_count(ch) >= __ch_buffer_size(ch))
#define __ch_pos_current(ch)    ((ch)->pos)
#define __ch_pos_index(ch, cnt)  \
    ((__ch_pos_current(ch) + (cnt)) % __ch_buffer_size(ch))
#define __ch_pos_tail(ch)       __ch_pos_index(ch, cr_chan_count(ch))
#define __ch_pos_next(ch)       __ch_pos_index(ch, 1)


/* 如果管道有效，且处于打开状态，返回真 */
static inline int __ch_is_opened_valid(cr_channel_t *ch)
{
    if (!ch) {
        return 0;
    }
    if (__ch_buffer_size(ch) > ch->buffer_size) {
        return 0;
    }
    if (__ch_pos_current(ch) >= ch->buffer_size) {
        return 0;
    }

    return cr_is_chan_open(ch);
}

/* 从管道的缓存中取出一项数据，并将其从管道中删除 */
static inline int __ch_buffer_pop(cr_channel_t *ch, void **buffer)
{
    if (__ch_buffer_empty(ch)) {
        return -1;
    }

    *buffer = ch->data[__ch_pos_current(ch)];
    ch->pos = __ch_pos_next(ch);
    ch->count -= 1;

    return 0;
}

/* 将一项数据加入到管道中 */
static inline int __ch_buffer_push(cr_channel_t *ch, void *data)
{
    if (__ch_buffer_full(ch)) {
        return -1;
    }

    ch->data[__ch_pos_tail(ch)] = data;
    ch->count += 1;

    return 0;
}

/* 尝试令当前协程作为一个管道的发送方 */
static inline int __ch_claim_sender(cr_channel_t *ch)
{
    if (ch->sender == NULL) {
        ch->sender = cr_self();
        return 0;
    }
    return (ch->sender == cr_self()) ? 0 : -1;
}

/* 尝试令当前协程作为一个管道的发送方 */
static inline int __ch_claim_receiver(cr_channel_t *ch)
{
    if (ch->receiver == NULL) {
        ch->receiver = cr_self();
        return 0;
    }
    return (ch->receiver == cr_self()) ? 0 : -1;
}


/* 初始化管道。缓存的大小会被调整到大于等于 1 */
int cr_channel_init(cr_channel_t *ch, unsigned int buffer_size)
{
    if (!ch) {
        return -1;
    }

    if (cr_waitable_init(ch->waitable) != 0) {
        return -1;
    }

    ch->receiver = NULL;
    ch->sender = NULL;
    ch->buffer_size = buffer_size ? buffer_size : 1;
    ch->count = 0;
    ch->pos = 0;
    memset(ch->data, 0, sizeof(void*) * buffer_size);
    __ch_flag_set_opened(ch);
    
    return 0;
}

/* 关闭一个管道 */
int cr_channel_close(cr_channel_t *ch)
{
    int ret = -1;
    if (!ch) {
        return -1;
    }

    __ch_flag_unset_opened(ch);
    ret = cr_waitable_notify_all(ch->waitable);
    cr_yield();
    return ret;
}

/* 创建一个给定大小缓存的管道 */
cr_channel_t *cr_channel_create(unsigned int buffer_size)
{
    cr_channel_t *ch = NULL;
    size_t ch_size = sizeof(cr_channel_t) + sizeof(void *) * buffer_size;

    if ((ch = malloc(ch_size)) == NULL) {
        return NULL;
    }

    if (cr_channel_init(ch, buffer_size) != 0) {
        free(ch);
        return NULL;
    }

    return ch;
}

/* 销毁一个管道，并释放这个管道所占的空间 */
int cr_channel_destroy(cr_channel_t *ch)
{
    int ret = -1;
    if (!ch) {
        return 0;
    }

    ret = cr_channel_close(ch);
    free(ch);
    return ret;
}

/* 从管道接收一项数据 */
int cr_channel_recv(cr_channel_t *ch, void **data)
{
    void *tmp_data = NULL;

    if (!data || !__ch_is_opened_valid(ch)) {
        return -1;
    }

    if (__ch_claim_receiver(ch) != 0) {
        return -1;
    }

    if (__ch_buffer_empty(ch)) {
        cr_await(ch->waitable);
        if (!__ch_is_opened_valid(ch)) {
            return -1;
        }
    }
    if (__ch_buffer_pop(ch, &tmp_data) != 0) {
        return -1;
    }
    *data = tmp_data;

    if (cr_is_waitable_busy(ch->waitable)) {
        cr_waitable_notify(ch->waitable);
    }

    return 0;
}

/* 向管道发送一项数据 */
int cr_channel_send(cr_channel_t *ch, void *data)
{
    int ret = -1;
    void *tmp_data = NULL;

    if (!__ch_is_opened_valid(ch)) {
        return -1;
    }

    if (__ch_claim_sender(ch) != 0) {
        return -1;
    }

    /* 尝试清空缓冲 */
    while (__ch_buffer_full(ch)) {
        cr_channel_flush(ch);
    }

    return __ch_buffer_push(ch, data);
}

/* 通过唤醒等待接收 data 的协程，尝试将一个管道清空 */
int cr_channel_flush(cr_channel_t *ch)
{
    unsigned int tmp = 0;

    if (!__ch_is_opened_valid(ch)) {
        return -1;
    }

    if (__ch_claim_sender(ch) != 0) {
        return -1;
    }

    tmp = cr_chan_count(ch);
    if (tmp > 0) {
        for (unsigned int i = 0; i < tmp; i++) {
            if (cr_waitable_notify(ch->waitable) != 0) {
                break;
            }
        }
        return cr_await(ch->waitable);
    }

    return 0;
}
