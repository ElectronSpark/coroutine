/* 协程和协程控制块 */
#ifndef __CR_TASK_H__
#define __CR_TASK_H__

#include <cr_types.h>
#include <cr_waitqueue.h>


/* 全局协程表相关操作 */
extern cr_gct_t cr_global_control_table;
#define cr_global()     (&cr_global_control_table)
#define cr_self()       (cr_global()->current_task)
#define cr_main()       (cr_global()->main_task)
#define cr_set_current_task(task)   \
    do { cr_global()->current_task = task; } while (0)

/* 用于判断协程框架是否处于有效状态 */
#define cr_is_eventloop_valid() (cr_global()->flag.valid)


/* 协程控制块状态查看 */
#define cr_task_is_main(task)   ((task) == cr_main())
#define cr_task_is_ready(task)  \
    cr_is_task_in_waitqueue((task), cr_global()->ready_queue)


/* 协程控制块相关操作 */
int cr_init_main(cr_task_t *task);
int cr_task_init(cr_task_t *task, cr_function_t entry, void *arg);
cr_task_t *cr_task_create(cr_function_t entry, void *arg);
int cr_task_destroy(cr_task_t *task);
int cr_task_cancel(cr_task_t *task);
void cr_task_exit(void);

int cr_resume(cr_task_t *task);
int cr_yield(void);

int cr_suspend(cr_task_t *task);
int cr_wakeup(cr_task_t *task, int err);


#endif /* __CR_TASK_H__ */
