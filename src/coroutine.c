#include <unistd.h>

#include <coroutine.h>
#include <cpu_specific.h>


/* 整个框架里唯一一个全局协程表 */
cr_gct_t cr_global_control_table;


/* 初始化整个协程框架 */
int cr_init(void)
{
    cr_waitable_init(cr_global()->ready_queue);
    cr_global()->task_count = 0;

    if (cr_init_main(cr_main()) != 0) {
        return -1;
    }

    cr_global()->current_task = cr_global()->main_task;
    cr_global()->flag.valid = 1;

    return 0;
}

/* 启动协程调度器，并一直执行直到所有协程都结束 */
int cr_wait_event_loop(void)
{
    cr_task_t *task = NULL;

    if (!cr_is_eventloop_valid()) {
        return -1;
    }

    /* 不断地进行协程调度，直到所有协程都结束 */
    while (cr_global()->task_count > 0) {

        /* 从就绪队列中取出一个 */
        task = cr_waitable_get(cr_global()->ready_queue);
        
        /* 如果当前就绪队列为空，等待接收到操作系统的信号 */
        if (!task) {
            pause();
            continue;
        }

        /* 将从就绪队列中取出的协程控制块放到就绪队列的最末尾 */
        cr_waitable_put_off(cr_global()->ready_queue, task);
        /* 恢复选择到的协程的执行 */
        cr_resume(task);
    }

    return 0;
}
