#include <rbtree.h>
#include <coroutine.h>
#include <cr_pool.h>
#include <cr_waitqueue.h>
#include <cr_event.h>


static CR_POOL_DECLARE(__event_node_pool, sizeof(cr_event_node_t),  \
                       CR_DEFAULT_POOL_SIZE, CR_POOL_EVENT_NODE_WATERMARK);


/* 检查事件控制块的合法性 */
#define __check_event_valid(event)  (!!(event) && !!(event)->flag.active)
/* 检查事件节点控制块的合法性 */
#define __check_enode_valid(enode)  (!!(enode) && !!(enode)->event)


/* 分配和释放事件节点 */
#define __event_node_alloc()    \
    cr_pool_block_alloc(&__event_node_pool)
#define __event_node_free(__ptr)    \
    cr_pool_block_free(&__event_node_pool, (__ptr))


/* 获得一个事件节点所在的事件控制块 */
static inline cr_event_t *__get_node_event(cr_event_node_t *enode)
{
    if (!!enode && __check_event_valid(enode->event)) {
        return enode->event;
    }
    return NULL;
}

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

/* 在事件控制块的红黑树内删除事件节点 */
static inline void __do_rb_delete(cr_event_t *event, cr_event_node_t *node)
{
    if (event->last_found == node) {
        event->last_found = NULL;
    }
    rb_erase(node->rb_node, event->nodes);
    RB_CLEAR_NODE(node->rb_node);
    event->count -= 1;
}

/* 让当前协程等待一个事件，这个事件必须没有协程正在等待 */
static inline int __do_await(cr_event_t *event,
                             cr_event_node_t *node,
                             void **err)
{
    if (!cr_is_waitqueue_empty(node->waitqueue)) {
        return -1;
    }
    return cr_await(node->waitqueue, err);
}

/* 唤醒一个事件节点上的协程，并向被唤醒协程传递给定数据 */
static inline int __do_notify(cr_event_t *event,
                              cr_event_node_t *node,
                              void *err)
{
    if (cr_is_waitqueue_empty(node->waitqueue)) {
        return 0;
    }
    return cr_waitqueue_notify(node->waitqueue, err);
}

/* 唤醒等待一个事件的所有任务，并向所有被唤醒的协程传递相同的给定数据 */
static inline int __do_notify_all(cr_event_t *event, void *err) {
    cr_event_node_t *pos = NULL;
    cr_event_node_t *n = NULL;
    struct rb_root *root = event->nodes;
    int ret = 0;

    rbtree_postorder_for_each_entry_safe(pos, n, root, rb_node[0]) {
        if (__do_notify(event, pos, err) != 0) {
            ret = -1;
        }
    }
    return ret;
}

/* 在将事件节点插入事件控制块前初始化该节点 */
static inline int __init_event_node(cr_event_node_t *node, cr_event_t *event,
                                    cr_eid_t eid, cr_task_t *task, void *data)
{
    node->eid = eid;
    node->event = event;
    node->data = data;
    RB_CLEAR_NODE(node->rb_node);
    return cr_waitqueue_init(node->waitqueue);
}


/* 初始化事件控制块 */
int cr_event_init(cr_event_t *event)
{
    if (!event) {
        return -1;
    }

    if (cr_waitqueue_init(event->waitqueue) != 0) {
        return -1;
    }
    event->count = 0;
    event->nodes[0] = RB_ROOT;
    event->flag.active = 1;
    
    return 0;
}

/* 关闭一个事件 */
int cr_event_destroy(cr_event_t *event)
{
    if (!__check_event_valid(event)) {
        return -1;
    }

    event->flag.active = 0;
    while (__do_notify_all(event, cr_err2ptr(CR_ERR_CLOSE)) == 0);
    return 0;
}

/* 将事件节点插入插入事件控制块的红黑树中 */
cr_event_node_t *cr_event_add(cr_event_t *event, cr_eid_t eid)
{
    struct rb_node *rb_parent = NULL;
    struct rb_node **rb_link = NULL;
    cr_event_node_t *node = NULL;

    if (event->last_found && event->last_found->eid == eid) {
        return NULL;
    }
    rb_link = __find_rb_link(event, eid, &rb_parent);
    if (*rb_link != NULL) {
        return NULL;
    }
    if ((node = __event_node_alloc()) == NULL) {
        return NULL;
    }
    if (__init_event_node(node, event, eid, cr_self(), NULL) != 0) {
        __event_node_free(node);
        return NULL;
    }
    rb_link_node(node->rb_node, rb_parent, rb_link);
    rb_insert_color(node->rb_node, event->nodes);
    event->count += 1;
    return node;
}

/* 通过事件号在事件控制块内找到事件节点 */
cr_event_node_t *cr_event_find(cr_event_t *event, cr_eid_t eid)
{
    struct rb_node **link = NULL;
    cr_event_node_t *node = NULL;

    if (event == NULL) {
        return NULL;
    }
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

/* 将事件节点移出事件控制块 */
int cr_event_remove(cr_event_node_t *enode, void *err)
{
    cr_event_t *event = __get_node_event(enode);
    if (!event) {
        return -1;
    }

    __do_rb_delete(event, enode);
    enode->event = NULL;
    if (__do_notify(event, enode, err) != 0) {
        return -1;
    }
    __event_node_free(enode);
    return 0;
}

/* 令当前协程等待一个事件 */
int cr_event_wait(cr_event_node_t *enode, void *data, void **err)
{
    cr_event_t *event = __get_node_event(enode);
    int ret = -1;
    if (!event) {
        return -1;
    }
    if (!cr_is_waitqueue_empty(enode->waitqueue)) {
        return -1;
    }
    
    enode->data = data;
    if ((ret = __do_await(event, enode, err)) != 0) {
        return ret;
    }
    return __check_enode_valid(enode) ? ret : -1;
}

/* 获得等待事件的协程传递给事件处理函数的数据 */
int cr_event_get_data(cr_event_node_t *enode, void **data)
{
    if (!enode || !data) {
        return -1;
    }
    *data = enode->data;
    return 0;
}

/* 唤醒一个等待事件的协程，并将给定数据传给该协程 */
int cr_event_notify(cr_event_node_t *enode, void *data)
{
    cr_event_t *event = __get_node_event(enode);
    if (!event) {
        return -1;
    }
    enode->data;
    return __do_notify(event, enode, cr_err2ptr(CR_ERR_OK));
}

/* 唤醒等待事件的所有协程，并将给定数据传递给它们 */
int cr_event_notify_all(cr_event_t *event, void *data)
{
    if (!__check_event_valid(event)) {
        return -1;
    }
    return __do_notify_all(event, data);
}
