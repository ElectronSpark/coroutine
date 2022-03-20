/* 协程和协程控制块 */
#ifndef __CR_TASK_H__
#define __CR_TASK_H__

#include <cr_types.h>


/* 全局协程表相关操作 */
extern cr_gct_t cr_global_control_table;
#define cr_global()     (&cr_global_control_table)
#define cr_self()       (cr_global()->current_task)
#define cr_main()       (cr_global()->main_task)


/* 协程控制块状态查看 */
#define cr_task_is_main(task)   ((task)->flag.main)
#define cr_task_is_alive(task)   ((task)->flag.alive)
#define cr_task_is_ready(task)   ((task)->flag.ready)

/* 协程控制块状态设置与清除 */
#define __cr_task_change_flag(_task, _flag, _value)    \
    do { (task)->flag._flag = _value; } while (0)

#define cr_task_set_main(task)      __cr_task_change_flag(task, main, 1)
#define cr_task_set_alive(task)     __cr_task_change_flag(task, alive, 1)
#define cr_task_set_ready(task)     __cr_task_change_flag(task, ready, 1)

#define cr_task_unset_main(task)      __cr_task_change_flag(task, main, 0)
#define cr_task_unset_alive(task)     __cr_task_change_flag(task, alive, 0)
#define cr_task_unset_ready(task)     __cr_task_change_flag(task, ready, 0)


/* 协程队列操作 */
void cr_waitable_init(cr_waitable_t *waitable);
void cr_waitable_push(cr_waitable_t *waitable, cr_task_t *task);
void cr_waitable_push_tail(cr_waitable_t *waitable, cr_task_t *task);
cr_task_t *cr_waitable_get(cr_waitable_t *waitable);
cr_task_t *cr_waitable_get_tail(cr_waitable_t *waitable);
cr_task_t *cr_waitable_pop(cr_waitable_t *waitable);
cr_task_t *cr_waitable_pop_tail(cr_waitable_t *waitable);


/* 协程控制块相关操作 */
int cr_init_main(cr_task_t *task);
cr_task_t *cr_task_create(cr_function_t entry, void *arg);
int cr_task_init(cr_task_t *task);
int cr_task_cancel(cr_task_t *task);

int cr_resume(void);
int cr_yield(void);

int cr_await(cr_waitable_t *waitable);
int cr_wakeup(cr_task_t *task);


#endif /* __CR_TASK_H__ */
