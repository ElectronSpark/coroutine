/* 整个协程框架数据类型的定义 */
#ifndef __CR_TYPES_H__
#define __CR_TYPES_H__

#include <stddef.h>

#include <rbtree.h>
#include <cpu_types.h>
#include <list_types.h>


/* 协程入口函数原型 */
typedef void (*cr_function_t)(void *param);

/* 等待队列，协程间协作的基础 */
struct cr_waitqueue_struct {
    struct list_head    wait_queue[1];
};
typedef struct cr_waitqueue_struct   cr_waitqueue_t;

/* 协程控制块 */
struct cr_task_struct {
    struct list_head    list_head[1];
    cr_waitqueue_t      *cur_queue;
    int                 cr_errno;
    int                 epoll_events;
    
    void    *stack_base;
    size_t  stack_size;
    void    *arg;
    cr_context_t    regs[1];
    cr_function_t   entry;
};
typedef struct cr_task_struct       cr_task_t;

/* 全局协程表 */
struct cr_global_control_table_struct {
    cr_waitqueue_t       ready_queue[1];
    cr_waitqueue_t       cancel_queue[1];
    cr_function_t       idle_handler;
    int     task_count;
    int     cancel_count;
    int     sched_count;
    struct {
        int     valid: 1;
    } flag;

    struct cr_task_struct   main_task[1];
    struct cr_task_struct   *current_task;
};
typedef struct cr_global_control_table_struct   cr_gct_t;

/* 信号量 */
struct cr_semaphore_struct {
    int             value;
    int             limit;
    struct {
        int     valid: 1;
        int     opened: 1;
    } flag;
    cr_waitqueue_t   waitqueue[1];
};
typedef struct cr_semaphore_struct  cr_sem_t;

/* 通道 */
struct cr_channel_struct {
    cr_waitqueue_t   waitqueue[1];
    cr_task_t       *sender;
    cr_task_t       *receiver;

    struct {
        int     opened: 1;
    }   flag;
    
    unsigned int    buffer_size;    /* 缓冲区能容纳的项个数 */
    unsigned int    pos;    /* 缓冲区的当前位置指针 */
    unsigned int    count;  /* 缓冲区当前所存放的项个数 */
    void            *data[0];
};
typedef struct cr_channel_struct    cr_channel_t;


/* 内存池相关 */
typedef struct cr_pool_struct   cr_pool_t;
struct cr_pool_node_struct {
    struct list_head    list_head[1];
    struct rb_node      rb_node[1];     /* 用于定位内存池节点的红黑树根 */
    cr_pool_t   *pool;
    
    size_t  block_size;
    size_t  node_size;
    int     block_total;
    int     block_free;
    int     block_used;

    void    *first_free;
    void    *buffer;
};
typedef struct cr_pool_node_struct  cr_pool_node_t;

struct cr_pool_struct {
    struct rb_root      rb_root[1];     /* 用于定位内存池节点的红黑树根 */
    struct list_head    full[1];
    struct list_head    partial[1];
    struct list_head    free[1];
    
    int     full_nodes;
    int     partial_nodes;
    int     free_nodes;
    int     block_total;
    int     block_used;
    int     block_free;

    size_t  block_size;
    size_t  node_size;
    int     water_mark;
};
typedef struct cr_pool_struct   cr_pool_t;

/* 事件 */
/* 作为事件的 id，在事件被触发时会根据事件的 id来定位需要唤醒的任务 */
typedef unsigned long   cr_eid_t;

typedef struct cr_event_control_struct  cr_event_t;
struct cr_event_node_struct {
    struct rb_node  rb_node[1];
    cr_waitqueue_t   waitqueue[1];
    cr_eid_t        eid;
    cr_event_t      *event;
    void            *data;
};
typedef struct cr_event_node_struct cr_event_node_t;

struct cr_event_control_struct {
    struct rb_root  nodes[1];
    int             count;
    cr_waitqueue_t   waitqueue[1];
    struct {
        int     active: 1;
    } flag;
    cr_event_node_t *last_found;
};
typedef struct cr_event_control_struct  cr_event_t;

/* 暂时储存一个 fd 的相关信息 */
struct cr_fditem_struct {
    int             fd;
    cr_waitqueue_t  wait_close[1];
    cr_waitqueue_t  wait_queue[1];
};
typedef struct cr_fditem_struct cr_fd_t;

#endif /* __CR_TYPES_H__ */
