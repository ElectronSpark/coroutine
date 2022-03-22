/* 协程和协程控制块 */
#include <coroutine.h>
#include <cr_pool.h>
#include <cpu_specific.h>


#define CR_TASK_STACK_SIZE          (1UL << 14)
#define CR_DEFAULT_POOL_SIZE        CR_POOL_NODE_SIZE_MAX
#define CR_POOL_STACK_WATERMARK     32
#define CR_POOL_TCB_WATERMARK       (CR_DEFAULT_POOL_SIZE / sizeof(cr_task_t))
#define CR_POOL_WAITABLE_WATERMARK  (CR_DEFAULT_POOL_SIZE / sizeof(cr_waitable_t))


static CR_POOL_DECLARE(__task_stack_pool, CR_TASK_STACK_SIZE,   \
                       CR_DEFAULT_POOL_SIZE, CR_POOL_STACK_WATERMARK);
static CR_POOL_DECLARE(__task_tcb_pool, sizeof(cr_task_t),  \
                       CR_DEFAULT_POOL_SIZE, CR_POOL_TCB_WATERMARK);


/* 初始化协程控制块 */
static void __tcb_init(cr_task_t *task, cr_function_t entry, void *arg,
                      void *stack_base, size_t stack_size)
{
    INIT_LIST_HEAD(task->list_head);
    task->stack_base = stack_base;
    task->stack_size = stack_size;
    task->arg = arg;
    task->entry = entry;

    cr_task_unset_main(task);
    cr_task_unset_alive(task);
    cr_task_unset_ready(task);
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


/* 初始化主协程 */
int cr_init_main(cr_task_t *task)
{
    if (!task) {
        return -1;
    }

    __tcb_init(task, NULL, NULL, NULL, NULL);
    cr_task_set_main(task);
    cr_task_set_alive(task);
    cr_task_set_ready(task);

    return 0;
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

    __tcb_init(new_task, entry, arg, new_task->stack_base, 
               new_task->stack_size);
    cr_context_init(new_task->regs, new_task->stack_base,
                    new_task->stack_size, (void *)__cr_task_entry_func, arg,
                    (void *)entry, (void *)new_task);
}
