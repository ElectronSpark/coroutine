/* 协程事件，用于处理来自外部的事件 */
#ifndef __CR_EVENT_H__
#define __CR_EVENT_H__

#include <rbtree.h>
#include <cr_types.h>
#include <cr_waitable.h>


/* 静态初始化一个事件控制块 */
#define CR_EVENT_INIT(__name)    {   \
    .nodes = { RB_ROOT },   \
    .count = 0, \
    .waitable = { CR_WAITABLE_INIT((__name).waitable[0]) }, \
    .flag = { .active = 1 },    \
    .last_found = NULL  \
}
/* 静态声明一个事件控制块 */
#define CR_EVENT_DECLARE(name) \
    cr_event_t name = CR_EVENT_INIT(name)


int cr_event_init(cr_event_t *event);
int cr_event_close(cr_event_t *event, void *data);
int cr_event_wait(cr_event_t *event, cr_eid_t eid,
                  void *data, void **ret_data);
int cr_event_get_data(cr_event_t *event, cr_eid_t eid, void **data);
int cr_event_notify(cr_event_t *event, cr_eid_t eid, void *data);
int cr_event_notify_all(cr_event_t *event, void *data);


#endif /* __CR_EVENT_H__ */
