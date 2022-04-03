#include <coroutine.h>
#include <cr_waitqueue.h>


/* 初始化一个等待队列 */
int cr_waitqueue_init(cr_waitqueue_t *waitqueue)
{
    if (!waitqueue) {
        return -CR_ERR_INVAL;
    }

    INIT_LIST_HEAD(waitqueue->wait_queue);
    return CR_ERR_OK;
}


/* 为协程加入等待队列做准备，准备成功返回 0，失败返回 -1 */
static int __cr_waitqueue_push_prepare(cr_waitqueue_t *waitqueue, cr_task_t *task)
{
    if (!waitqueue || !task || task->cur_queue) {
        return -CR_ERR_INVAL;
    }
    if (!list_empty(task->list_head)) {
        return -CR_ERR_INVAL;
    }
    return CR_ERR_OK;
}

/* 让协程加入等待队列队首 */
int cr_waitqueue_push(cr_waitqueue_t *waitqueue, cr_task_t *task)
{
    int ret = CR_ERR_OK;
    if ((ret = __cr_waitqueue_push_prepare(waitqueue, task)) != 0) {
        return ret;
    }
    list_add(task->list_head, waitqueue->wait_queue);
    task->cur_queue = waitqueue;
    return CR_ERR_OK;
}

/* 让协程加入等待队列队尾 */
int cr_waitqueue_push_tail(cr_waitqueue_t *waitqueue, cr_task_t *task)
{
    int ret = CR_ERR_OK;
    if ((ret = __cr_waitqueue_push_prepare(waitqueue, task)) != 0) {
        return ret;
    }
    list_add_tail(task->list_head, waitqueue->wait_queue);
    task->cur_queue = waitqueue;
    return CR_ERR_OK;
}


/* 为协程移出等待队列或改变在队列中的位置做准备，准备成功返回 0，失败返回 -1 */
static int __cr_waitqueue_move_prepare(cr_waitqueue_t *waitqueue, cr_task_t *task)
{
    if (!waitqueue || !task || !task->cur_queue) {
        return -CR_ERR_INVAL;
    }
    if (!cr_is_task_in_waitqueue(task, waitqueue)) {
        return -CR_ERR_INVAL;
    }
    if (list_empty(task->list_head)) {
        return -CR_ERR_INVAL;
    }
    return CR_ERR_OK;
}

/* 将一个已经位于等待队列的协程移动到队尾 */
int cr_waitqueue_put_off(cr_waitqueue_t *waitqueue, cr_task_t *task)
{
    int ret = CR_ERR_OK;
    if ((ret = __cr_waitqueue_move_prepare(waitqueue, task)) != 0) {
        return ret;
    }

    list_move_tail(task->list_head, waitqueue->wait_queue);
    return CR_ERR_OK;
}

/* 获得位于等待队列队首的协程 */
cr_task_t *cr_waitqueue_get(cr_waitqueue_t *waitqueue)
{
    cr_task_t *ret = NULL;

    if (!waitqueue || cr_is_waitqueue_empty(waitqueue)) {
        return NULL;
    }

    ret = list_first_entry_or_null(waitqueue->wait_queue, cr_task_t, 
                                   list_head[0]);
    return ret;
}

/* 获得位于等待队列队尾的协程 */
cr_task_t *cr_waitqueue_get_tail(cr_waitqueue_t *waitqueue)
{
    cr_task_t *ret = NULL;

    if (!waitqueue || cr_is_waitqueue_empty(waitqueue)) {
        return NULL;
    }

    if (waitqueue->wait_queue->prev != waitqueue->wait_queue) {
        ret = list_last_entry(waitqueue->wait_queue, cr_task_t, list_head[0]);
    } else {
        ret = NULL;
    }
    return ret;
}

/* 将一个协程从等待队列中移除 */
int cr_waitqueue_remove(cr_waitqueue_t *waitqueue, cr_task_t *task)
{
    int ret = CR_ERR_OK;
    if ((ret = __cr_waitqueue_move_prepare(waitqueue, task)) != 0) {
        return ret;
    }

    list_del_init(task->list_head);
    task->cur_queue = NULL;
    return CR_ERR_OK;
}

/* 获得等待队列队首的协程，并将其弹出 */
cr_task_t *cr_waitqueue_pop(cr_waitqueue_t *waitqueue)
{
    cr_task_t *ret = cr_waitqueue_get(waitqueue);
    if (cr_waitqueue_remove(waitqueue, ret) != 0) {
        return NULL;
    }
    return ret;
}

/* 获得等待队列队尾的协程，并将其弹出 */
cr_task_t *cr_waitqueue_pop_tail(cr_waitqueue_t *waitqueue)
{
    cr_task_t *ret = cr_waitqueue_get_tail(waitqueue);
    if (cr_waitqueue_remove(waitqueue, ret) != 0) {
        return NULL;
    }
    return ret;
}

/* 唤醒等待队列队头的一个协程 */
int cr_waitqueue_notify(cr_waitqueue_t *waitqueue, int ret)
{
    cr_task_t *task = cr_waitqueue_pop(waitqueue);
    return cr_wakeup(task, ret);
}

/* 唤醒位于等待队列的所有协程 */
int cr_waitqueue_notify_all(cr_waitqueue_t *waitqueue, int ret)
{
    cr_task_t *task = NULL;
    if (!waitqueue) {
        return -CR_ERR_INVAL;
    }

    while ((task = cr_waitqueue_pop(waitqueue)) != NULL) {
        cr_wakeup(task, ret);
    }
    if (cr_is_waitqueue_empty(waitqueue)) {
        return CR_ERR_OK;
    } else {
        return -CR_ERR_FAIL;
    }
}


/* 为协程加入等待队列并挂起做准备，准备成功返回 0，失败返回 -1 */
static int __cr_await_prepare(cr_waitqueue_t *waitqueue, cr_task_t *task)
{
    if (!task || !waitqueue) {
        return -CR_ERR_INVAL;
    }

    if (!cr_task_is_ready(task)) {
        return -CR_ERR_INVAL;
    }

    return cr_suspend(task);
}

/* 让当前协程加入等待队列并挂起 */
int cr_await(cr_waitqueue_t *waitqueue)
{
    cr_task_t *task = cr_self();
    int ret = CR_ERR_OK;

    if ((ret = __cr_await_prepare(waitqueue, task)) == 0) {
        cr_waitqueue_push_tail(waitqueue, task);
        return cr_sched();
    }
    
    return ret;
}
