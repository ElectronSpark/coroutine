/* 协程和协程控制块 */
#ifndef __CR_TASK_H__
#define __CR_TASK_H__

#include <cr_types.h>


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
#define cr_task_is_main(task)   ((task)->flag.main)
#define cr_task_is_alive(task)   ((task)->flag.alive)
#define cr_task_is_ready(task)   ((task)->flag.ready)

/* 协程控制块状态设置与清除 */
#define __cr_task_change_flag(_task, _flag, _value)    \
    do { (_task)->flag._flag = _value; } while (0)

#define cr_task_set_main(task)      __cr_task_change_flag(task, main, 1)
#define cr_task_set_alive(task)     __cr_task_change_flag(task, alive, 1)
#define cr_task_set_ready(task)     __cr_task_change_flag(task, ready, 1)

#define cr_task_unset_main(task)      __cr_task_change_flag(task, main, 0)
#define cr_task_unset_alive(task)     __cr_task_change_flag(task, alive, 0)
#define cr_task_unset_ready(task)     __cr_task_change_flag(task, ready, 0)


/* 协程控制块相关操作 */
int cr_init_main(cr_task_t *task);
int cr_task_init(cr_task_t *task, cr_function_t entry, void *arg);
cr_task_t *cr_task_create(cr_function_t entry, void *arg);
int cr_task_destroy(cr_task_t *task);
int cr_task_cancel(cr_task_t *task);
void cr_task_exit(void);

int cr_resume(cr_task_t *task);
int cr_sched(void **ret_data);

int cr_suspend(cr_task_t *task);
int cr_wakeup(cr_task_t *task, void *ret_data);


#endif /* __CR_TASK_H__ */
