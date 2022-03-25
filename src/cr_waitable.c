#include <coroutine.h>
#include <cr_waitable.h>


/* 初始化一个可等待实体 */
int cr_waitable_init(cr_waitable_t *waitable)
{
    if (!waitable) {
        return -1;
    }

    INIT_LIST_HEAD(waitable->wait_queue);
    return 0;
}


/* 为协程加入等待队列做准备，准备成功返回 0，失败返回 -1 */
static int __cr_waitable_push_prepare(cr_waitable_t *waitable, cr_task_t *task)
{
    if (!waitable || !task || task->cur_queue) {
        return -1;
    }
    if (!list_empty(task->list_head)) {
        return -1;
    }
    return 0;
}

/* 让协程加入等待实体的等待队列队首 */
int cr_waitable_push(cr_waitable_t *waitable, cr_task_t *task)
{
    if (__cr_waitable_push_prepare(waitable, task) != 0) {
        return -1;
    }
    list_add(task->list_head, waitable->wait_queue);
    task->cur_queue = waitable;
    return 0;
}

/* 让协程加入等待实体的等待队列队尾 */
int cr_waitable_push_tail(cr_waitable_t *waitable, cr_task_t *task)
{
    if (__cr_waitable_push_prepare(waitable, task) != 0) {
        return -1;
    }
    list_add_tail(task->list_head, waitable->wait_queue);
    task->cur_queue = waitable;
    return 0;
}


/* 为协程移出等待队列或改变在队列中的位置做准备，准备成功返回 0，失败返回 -1 */
static int __cr_waitable_move_prepare(cr_waitable_t *waitable, cr_task_t *task)
{
    if (!waitable || !task || !task->cur_queue) {
        return -1;
    }
    if (!cr_is_task_in_waitable(task, waitable)) {
        return -1;
    }
    if (list_empty(task->list_head)) {
        return -1;
    }
    return 0;
}

/* 将一个已经位于等待队列的协程移动到队尾 */
int cr_waitable_put_off(cr_waitable_t *waitable, cr_task_t *task)
{
    if (__cr_waitable_move_prepare(waitable, task) != 0) {
        return -1;
    }

    list_move_tail(task->list_head, waitable->wait_queue);
    return 0;
}

/* 获得位于等待队列队首的协程 */
cr_task_t *cr_waitable_get(cr_waitable_t *waitable)
{
    cr_task_t *ret = NULL;

    if (!waitable || !cr_is_waitable_busy(waitable)) {
        return NULL;
    }

    ret = list_first_entry_or_null(waitable->wait_queue, cr_task_t, 
                                   list_head[0]);
    return ret;
}

/* 获得位于等待队列队尾的协程 */
cr_task_t *cr_waitable_get_tail(cr_waitable_t *waitable)
{
    cr_task_t *ret = NULL;

    if (!waitable || !cr_is_waitable_busy(waitable)) {
        return NULL;
    }

    if (waitable->wait_queue->prev != waitable->wait_queue) {
        ret = list_last_entry(waitable->wait_queue, cr_task_t, list_head[0]);
    } else {
        ret = NULL;
    }
    return ret;
}

/* 将一个协程从可等待实体中移除 */
int cr_waitable_remove(cr_waitable_t *waitable, cr_task_t *task)
{
    if (__cr_waitable_move_prepare(waitable, task) != 0) {
        return -1;
    }
    list_del_init(task->list_head);
    task->cur_queue = NULL;
    return 0;
}

/* 获得等待队列队首的协程，并将其弹出 */
cr_task_t *cr_waitable_pop(cr_waitable_t *waitable)
{
    cr_task_t *ret = cr_waitable_get(waitable);
    if (cr_waitable_remove(waitable, ret) != 0) {
        return NULL;
    }
    return ret;
}

/* 获得等待队列队尾的协程，并将其弹出 */
cr_task_t *cr_waitable_pop_tail(cr_waitable_t *waitable)
{
    cr_task_t *ret = cr_waitable_get_tail(waitable);
    if (cr_waitable_remove(waitable, ret) != 0) {
        return NULL;
    }
    return ret;
}

/* 唤醒等待可等待实体的一个协程 */
int cr_waitable_notify(cr_waitable_t *waitable)
{
    cr_task_t *task = cr_waitable_pop(waitable);
    return cr_wakeup(task);
}

/* 唤醒等待可等待实体的所有协程 */
int cr_waitable_notify_all(cr_waitable_t *waitable)
{
    cr_task_t *task = NULL;
    if (!waitable) {
        return -1;
    }

    while ((task = cr_waitable_pop(waitable)) != NULL) {
        cr_wakeup(task);
    }
    return cr_is_waitable_busy(waitable) ? -1 : 0;
}


/* 为协程加入等待队列并挂起做准备，准备成功返回 0，失败返回 -1 */
static int __cr_await_prepare(cr_waitable_t *waitable, cr_task_t *task)
{
    if (!task || !waitable) {
        return -1;
    }

    if (!cr_task_is_ready(task)) {
        return -1;
    }

    return cr_suspend(task);
}

/* 让当前协程加入一个可等待实体的等待队列并挂起 */
int cr_await(cr_waitable_t *waitable)
{
    cr_task_t *task = cr_self();

    if (__cr_await_prepare(waitable, task) == 0) {
        cr_waitable_push_tail(waitable, task);
        return cr_yield();
    }
    
    return -1;
}
