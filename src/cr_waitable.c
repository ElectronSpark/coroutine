#include <coroutine.h>
#include <cr_waitable.h>


int cr_waitable_init(cr_waitable_t *waitable)
{
    if (!waitable) {
        return -1;
    }

    INIT_LIST_HEAD(waitable->wait_queue);
    return 0;
}


static int __cr_waitable_push_prepare(cr_waitable_t *waitable, cr_task_t *task)
{
    if (!waitable || !task) {
        return -1;
    }
    if (!list_empty_careful(task->list_head)) {
        return -1;
    }
    return 0;
}

int cr_waitable_push(cr_waitable_t *waitable, cr_task_t *task)
{
    if (__cr_waitable_push_prepare(waitable, task) != 0) {
        return -1;
    }
    list_add(task->list_head, waitable->wait_queue);
    return 0;
}

int cr_waitable_push_tail(cr_waitable_t *waitable, cr_task_t *task)
{
    if (__cr_waitable_push_prepare(waitable, task) != 0) {
        return -1;
    }
    list_add_tail(task->list_head, waitable->wait_queue);
    return 0;
}


static int __cr_waitable_move_prepare(cr_waitable_t *waitable, cr_task_t *task)
{
    if (!waitable || !task) {
        return -1;
    }
    if (list_empty_careful(task->list_head)) {
        return -1;
    }
    return 0;
}

int cr_waitable_put_off(cr_waitable_t *waitable, cr_task_t *task)
{
    if (__cr_waitable_move_prepare(waitable, task) != 0) {
        return -1;
    }

    list_move_tail(task->list_head, waitable->wait_queue);
    return 0;
}

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

int cr_waitable_remove(cr_waitable_t *waitable, cr_task_t *task)
{
    if (__cr_waitable_move_prepare(waitable, task) != 0) {
        return -1;
    }
    list_del_init(task->list_head);
    return 0;
}

cr_task_t *cr_waitable_pop(cr_waitable_t *waitable)
{
    cr_task_t *ret = cr_waitable_get(waitable);
    if (cr_waitable_remove(waitable, ret) != 0) {
        return NULL;
    }
    return ret;
}

cr_task_t *cr_waitable_pop_tail(cr_waitable_t *waitable)
{
    cr_task_t *ret = cr_waitable_get_tail(waitable);
    if (cr_waitable_remove(waitable, ret) != 0) {
        return NULL;
    }
    return ret;
}

int cr_waitable_notify(cr_waitable_t *waitable)
{
    cr_task_t *task = cr_waitable_pop(waitable);
    return cr_wakeup(task);
}

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

int cr_await(cr_waitable_t *waitable)
{
    cr_task_t *task = cr_self();

    if (__cr_await_prepare(waitable, task) == 0) {
        cr_waitable_push_tail(waitable, task);
    }

    return cr_yield();
}
