/* 协程和协程控制块 */
#include <coroutine.h>
#include <cr_pool.h>
#include <cpu_specific.h>


static CR_POOL_DECLARE(__task_stack_pool, CR_TASK_STACK_SIZE,   \
                       CR_DEFAULT_POOL_SIZE, CR_POOL_STACK_WATERMARK);
static CR_POOL_DECLARE(__task_tcb_pool, sizeof(cr_task_t),  \
                       CR_DEFAULT_POOL_SIZE, CR_POOL_TCB_WATERMARK);


/* 初始化协程控制块 */
static void __tcb_init(cr_task_t *task, cr_function_t entry, void *arg,
                       void *stack_base, size_t stack_size)
{
    INIT_LIST_HEAD(task->list_head);
    task->cur_queue = NULL;
    task->stack_base = stack_base;
    task->stack_size = stack_size;
    task->arg = arg;
    task->entry = entry;
    task->cr_errno = -CR_ERR_OK;
    task->epoll_events = 0;
}

/* 分配协程控制块和协程栈 */
static cr_task_t *__tcb_alloc(void)
{
    cr_task_t *new_task = NULL;
    void *stack = NULL;

    new_task = cr_pool_block_alloc(&__task_tcb_pool);
    if (!new_task) {
        return NULL;
    }
    stack = cr_pool_block_alloc(&__task_stack_pool);
    if (!stack) {
        cr_pool_block_free(&__task_tcb_pool, new_task);
        return NULL;
    }

    new_task->stack_base = stack;
    new_task->stack_size = CR_TASK_STACK_SIZE;

    return new_task;
}

/* 释放协程控制块和协程栈 */
static void __tcb_free(cr_task_t *task)
{
    if (task) {
        if (task->stack_base) {
            cr_pool_block_free(&__task_stack_pool, task->stack_base);
        }
        cr_pool_block_free(&__task_tcb_pool, task);
    }
}

/* C语言层面最顶层的协程入口函数，会调用真正的协程入口函数 */
static void __cr_task_entry_func(void *arg, cr_function_t entry, 
                                 cr_task_t *task)
{
    cr_set_current_task(task);
    entry(arg);
    cr_task_exit();
}

/* 保存当前协程上下文，并切换到下一个协程的上下文 */
static int __cr_task_switch_to(cr_task_t *task)
{
    /* 在栈里保存当前协程的协程控制块，用于返回当前协程后重新设置当前协程 */
    cr_task_t *self = cr_self();
    cr_task_t *last = NULL;
    __asm__ __volatile__("nop\n\t");    /* 防止编译器优化 */

    last = cr_switch_to(cr_self()->regs, task->regs, cr_self());
    cr_set_current_task(self);  /* 当前协程的协程控制块保存在当前栈的上下文中 */

    return CR_ERR_OK;
}

/* 在全局协程表中注册一个协程 */
static int __cr_task_register(cr_task_t *task)
{
    if (!task || !task->stack_base) {
        return -CR_ERR_INVAL;
    }

    if (cr_task_is_main(task)) {
        return -CR_ERR_INVAL;
    }

    cr_global()->task_count += 1;

    return CR_ERR_OK;
}

/* 在全局协程表中注销一个协程 */
static int __cr_task_unregister(cr_task_t *task)
{
    if (!task || !task->stack_base) {
        return -CR_ERR_INVAL;
    }

    if (cr_task_is_main(task)) {
        return -CR_ERR_INVAL;
    }

    cr_global()->task_count -= 1;

    return CR_ERR_OK;
}


/* 初始化主协程 */
int cr_init_main(cr_task_t *task)
{
    if (!task) {
        return -CR_ERR_INVAL;
    }

    __tcb_init(task, NULL, NULL, NULL, 0);

    return CR_ERR_OK;
}

/* 初始化一个一个协程 */
int cr_task_init(cr_task_t *task, cr_function_t entry, void *arg)
{
    int ret = CR_ERR_OK;

    if (!task || !entry || !task->stack_base) {
        return -CR_ERR_INVAL;
    }

    __tcb_init(task, entry, arg, task->stack_base, task->stack_size);
    cr_context_init(task->regs, task->stack_base, task->stack_size,
                    (void *)__cr_task_entry_func, arg, (void *)entry, 
                    (void *)task);

    if (__cr_task_register(task) != 0) {
        return -CR_ERR_FAIL;
    }

    return cr_wakeup(task, CR_ERR_OK);
}

/* 创建一个新的协程，并返回协程控制块 */
cr_task_t *cr_task_create(cr_function_t entry, void *arg)
{
    cr_task_t *new_task = NULL;

    if (!entry) {
        return NULL;
    }

    new_task = __tcb_alloc();
    if (!new_task) {
        return NULL;
    }

    if (cr_task_init(new_task, entry, arg) != 0) {
        __tcb_free(new_task);
        return NULL;
    }

    return new_task;
}

/* 销毁一个不处于存活状态的协程 */
int cr_task_destroy(cr_task_t *task)
{
    if (!task || !task->stack_base) {
        return -CR_ERR_INVAL;
    }

    if (cr_task_is_main(task)) {
        return -CR_ERR_INVAL;
    }

    __tcb_free(task);

    return CR_ERR_OK;
}

/* 取消某个协程，让其不出于存活状态 */
int cr_task_cancel(cr_task_t *task)
{
    int ret = CR_ERR_OK;
    /* 在挂起协程的过程中会检查给定的协程是否合法，然后将其从就绪队列中移除 */
    if ((ret = cr_suspend(task)) != 0) {
        return ret;
    }

    if ((ret = __cr_task_unregister(task)) != 0) {
        return ret;
    }

    cr_global()->cancel_count += 1;

    return cr_waitqueue_push_tail(cr_global()->cancel_queue, task);
}

/* 退出当前协程 */
void cr_task_exit(void)
{
    cr_task_cancel(cr_self());
    cr_yield();
}

/* 恢复某个协程的上下文 */
int cr_resume(cr_task_t *task)
{
    if (!cr_task_is_ready(task)) {
        return -CR_ERR_INVAL;
    }

    return __cr_task_switch_to(task);
}

/* 主动让出 CPU，切换回主协程 */
int cr_yield(void)
{
    int ret = CR_ERR_OK;
    if (!cr_task_is_main(cr_self())) {
        ret = __cr_task_switch_to(cr_main());
        if (ret == CR_ERR_OK) {
            ret = cr_self()->cr_errno;
        }
        return ret;
    }
    return -CR_ERR_INVAL;
}

/* 将一个协程挂起，移出就绪队列（不会发生协程上下文切换！）。 */
int cr_suspend(cr_task_t *task)
{
    int ret = CR_ERR_OK;
    if (!task || !task->stack_base) {
        return -CR_ERR_INVAL;
    }

    if (cr_task_is_main(task)) {
        return -CR_ERR_INVAL;
    }

    if (!cr_task_is_ready(task)) {
        return CR_ERR_OK;   /* 对于已经出于挂起状态的协程，什么也不做 */
    }

    if ((ret = cr_waitqueue_remove(cr_global()->ready_queue, task)) != 0) {
        return ret;
    }

    return CR_ERR_OK;
}

/* 唤醒一个协程，并将其加入就绪队列 */
int cr_wakeup(cr_task_t *task, int err)
{
    int ret = CR_ERR_OK;

    if (!task || !task->stack_base) {
        return -CR_ERR_INVAL;
    }

    if (cr_task_is_main(task)) {
        return -CR_ERR_INVAL;
    }

    if (cr_task_is_ready(task)) {
        return CR_ERR_OK;   /* 对于处于就绪状态的协程，什么也不做 */
    }

    if ((ret = cr_waitqueue_push_tail(cr_global()->ready_queue, task)) != 0) {
        return ret;
    }
    
    task->cr_errno = err;
    return CR_ERR_OK;
}
