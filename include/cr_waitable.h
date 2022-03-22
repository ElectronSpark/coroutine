/* 可等待实体的 API 实现 */
#ifndef __CR_WAITABLE_H__
#define __CR_WAITABLE_H__

#include <cr_types.h>


/* 判断是否有协程在等待一个可等待实体 */
#define cr_is_waitable_busy(wtb)  ((wtb))

void cr_waitable_init(cr_waitable_t *waitable);
void cr_waitable_push(cr_waitable_t *waitable, cr_task_t *task);
void cr_waitable_push_tail(cr_waitable_t *waitable, cr_task_t *task);
void cr_waitable_put_off(cr_waitable_t *waitable, cr_task_t *task);
cr_task_t *cr_waitable_get(cr_waitable_t *waitable);
cr_task_t *cr_waitable_get_tail(cr_waitable_t *waitable);
cr_task_t *cr_waitable_pop(cr_waitable_t *waitable);
cr_task_t *cr_waitable_pop_tail(cr_waitable_t *waitable);


#endif /* __CR_WAITABLE_H__ */
