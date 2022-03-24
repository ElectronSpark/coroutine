/* 通道，用于协程间传输数据 */
#ifndef __CR_CHANNEL_H__
#define __CR_CHANNEL_H__

#include <cr_types.h>


#define cr_chan_count(ch)       ((ch)->count)
#define cr_is_chan_open(ch)     ((ch)->flag.opened)
#define cr_is_chan_empty(ch)    (cr_chan_count(ch) == 0)


/* 管道的开关 */
int cr_channel_init(cr_channel_t *ch, unsigned int buffer_size);
int cr_channel_close(cr_channel_t *ch);

/* 利用内存池分配和释放管道 */
cr_channel_t *cr_channel_create(unsigned int buffer_size);
int cr_channel_destroy(cr_channel_t *ch);

/* 管道的收发操作 */
int cr_channel_send(cr_channel_t *ch, void *data);
int cr_channel_recv(cr_channel_t *ch, void **data);
int cr_channel_flush(cr_channel_t *ch);

#endif /* __CR_CHANNEL_H__ */
