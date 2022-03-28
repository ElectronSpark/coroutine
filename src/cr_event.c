#include <rbtree.h>
#include <coroutine.h>
#include <cr_waitable.h>
#include <cr_event.h>

/* 检查事件控制块的合法性 */
#define __check_event_valid(event)  (!!(event) && !!(event)->flag.active)

/* 找到给定事件号的节点在红黑树上的链接 */
static inline struct rb_node **__find_rb_link(cr_event_t *event,
                                              cr_eid_t eid,
                                              struct rb_node **ret_parent)
{
    struct rb_root *root = event->nodes;
    struct rb_node *rb_parent = NULL;
    struct rb_node **rb_link = &root->rb_node;
    cr_event_node_t *tmp_node = NULL;

    while (*rb_link != NULL) {
        rb_parent = *rb_link;
        tmp_node = rb_entry(rb_parent, cr_event_node_t, rb_node[0]);
        if (tmp_node->eid > eid) {
            rb_link = &rb_parent->rb_left;
        } else if (tmp_node->eid < eid) {
            rb_link = &rb_parent->rb_right;
        } else {
            event->last_found = tmp_node;
            break;
        }
    }

    if (ret_parent) {
        *ret_parent = rb_parent;
    }
    return rb_link;
}

/* 通过事件号在事件控制块的红黑树内找到事件节点 */
static inline cr_event_node_t *__find_event_node(cr_event_t *event,
                                                 cr_eid_t eid)
{
    struct rb_node **link = NULL;
    cr_event_node_t *node = NULL;

    if (event->last_found && event->last_found->eid == eid) {
        return event->last_found;
    }
    link = __find_rb_link(event, eid, NULL);
    if (*link == NULL) {
        return NULL;
    }
    node = rb_entry(*link, cr_event_node_t, rb_node[0]);
    if (node->eid != eid) {
        return NULL;
    }
    return node;
}

/* 将事件节点插入插入事件控制块的红黑树中 */
static inline int __do_rb_insert(cr_event_t *event, cr_event_node_t *node)
{
    struct rb_node *rb_parent = NULL;
    struct rb_node **rb_link = NULL;

    if (event->last_found && event->last_found->eid == node->eid) {
        return -1;
    }
    rb_link = __find_rb_link(event, node->eid, &rb_parent);
    if (*rb_link != NULL) {
        return -1;
    }
    rb_link_node(node->rb_node, rb_parent, rb_link);
    rb_insert_color(node->rb_node, event->nodes);
    event->count += 1;
    return 0;
}

/* 在事件控制块的红黑树内删除事件节点 */
static inline int __do_rb_delete(cr_event_t *event, cr_event_node_t *node)
{
    if (event->last_found == node) {
        event->last_found = NULL;
    }
    rb_erase(node->rb_node, event->nodes);
    RB_CLEAR_NODE(node->rb_node);
    event->count -= 1;
    return 0;
}

/* 唤醒一个事件节点上的协程，并向被唤醒协程传递给定数据 */
static inline int __do_notify(cr_event_t *event,
                              cr_event_node_t *node,
                              void *data)
{
    if (__do_rb_delete(event, node) != 0) {
        return -1;
    }
    node->ret_data = data;
    return cr_waitable_notify_one(event->waitable, node->task);
}

/* 唤醒等待一个事件的所有任务，并向所有被唤醒的协程传递相同的给定数据 */
static inline int __do_notify_all(cr_event_t *event, void *data) {
    cr_event_node_t *pos = NULL;
    cr_event_node_t *n = NULL;
    struct rb_root *root = event->nodes;
    int ret = 0;

    rbtree_postorder_for_each_entry_safe(pos, n, root, rb_node[0]) {
        if (__do_notify(event, pos, data) != 0) {
            ret = -1;
        }
    }
    return cr_yield() == 0 ? ret : -1;
}

/* 在将事件节点插入事件控制块前初始化该节点 */
static inline void __init_event_node(cr_event_node_t *node, cr_event_t *event,
                                     cr_eid_t eid, cr_task_t *task, void *data)
{
    node->eid = eid;
    node->event = event;
    node->data = data;
    node->ret_data = NULL;
    node->task = task;
    RB_CLEAR_NODE(node->rb_node);
}


/* 初始化事件控制块 */
int cr_event_init(cr_event_t *event)
{
    if (!event) {
        return -1;
    }

    if (cr_waitable_init(event->waitable) != 0) {
        return -1;
    }
    event->count = 0;
    event->nodes[0] = RB_ROOT;
    event->flag.active = 1;
    
    return 0;
}

/* 关闭一个事件 */
int cr_event_close(cr_event_t *event, void *data)
{
    if (!__check_event_valid(event)) {
        return -1;
    }

    event->flag.active = 0;
    while (__do_notify_all(event, data) != 0);
    return 0;
}

/* 令当前协程等待一个事件 */
int cr_event_wait(cr_event_t *event, cr_eid_t eid,
                  void *data, void **ret_data)
{
    cr_event_node_t event_node[1] = { 0 };
    int ret = -1;
    if (!__check_event_valid(event)) {
        return -1;
    }

    __init_event_node(event_node, event, eid, cr_self(), data);
    if (__do_rb_insert(event, event_node) != 0) {
        return -1;
    }
    ret = cr_await(event->waitable);
    if (ret == 0 && ret_data != NULL) {
        *ret_data = event_node->ret_data;
    }

    return ret;
}

/* 获得等待事件的协程传递给事件处理函数的数据 */
int cr_event_get_data(cr_event_t *event, cr_eid_t eid, void **data)
{
    cr_event_node_t *node = NULL;
    if (!__check_event_valid(event) || !data) {
        return -1;
    }
    if ((node = __find_event_node(event, eid)) == NULL) {
        return -1;
    }
    *data = node->data;
    return 0;
}

/* 唤醒一个等待事件的协程，并将给定数据传给该协程 */
int cr_event_notify(cr_event_t *event, cr_eid_t eid, void *data)
{
    cr_event_node_t *node = NULL;
    if (!__check_event_valid(event)) {
        return -1;
    }
    if ((node = __find_event_node(event, eid)) == NULL) {
        return -1;
    }
    return __do_notify(event, node, data);
}

/* 唤醒等待事件的所有协程，并将给定数据传递给它们 */
int cr_event_notify_all(cr_event_t *event, void *data)
{
    if (!__check_event_valid(event)) {
        return -1;
    }
    return __do_notify_all(event, data);
}
