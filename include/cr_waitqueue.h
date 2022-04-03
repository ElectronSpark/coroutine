/* 等待队列的 API 实现 */
#ifndef __CR_WAITQUEUE_H__
#define __CR_WAITQUEUE_H__

#include <cr_types.h>
#include <list.h>


/* 静态初始化等待队列 */
#define CR_WAITQUEUE_INIT(name)  \
    { LIST_HEAD_INIT((name).wait_queue[0]) }

/* 静态声明并初始化一个等待队列 */
#define CR_WAITQUEUE_DECLARE(name)   \
    cr_waitqueue_t name = CR_WAITQUEUE_INIT(name)


/* 判断是否有协程位于等待队列 */
#define cr_is_waitqueue_empty(wtb)      list_empty((wtb)->wait_queue)
#define cr_is_task_in_waitqueue(task, waitqueue)  \
    ((task)->cur_queue == (waitqueue))

int cr_waitqueue_init(cr_waitqueue_t *waitqueue);
int cr_waitqueue_push(cr_waitqueue_t *waitqueue, cr_task_t *task);
int cr_waitqueue_push_tail(cr_waitqueue_t *waitqueue, cr_task_t *task);
int cr_waitqueue_put_off(cr_waitqueue_t *waitqueue, cr_task_t *task);
cr_task_t *cr_waitqueue_get(cr_waitqueue_t *waitqueue);
cr_task_t *cr_waitqueue_get_tail(cr_waitqueue_t *waitqueue);
int cr_waitqueue_remove(cr_waitqueue_t *waitqueue, cr_task_t *task);
cr_task_t *cr_waitqueue_pop(cr_waitqueue_t *waitqueue);
cr_task_t *cr_waitqueue_pop_tail(cr_waitqueue_t *waitqueue);

int cr_waitqueue_notify(cr_waitqueue_t *waitqueue, int ret);
int cr_waitqueue_notify_all(cr_waitqueue_t *waitqueue, int ret);
int cr_await(cr_waitqueue_t *waitqueue);

#endif /* __CR_WAITQUEUE_H__ */
