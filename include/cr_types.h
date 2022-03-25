/* 整个协程框架数据类型的定义 */
#ifndef __CR_TYPES_H__
#define __CR_TYPES_H__

#include <stddef.h>

#include <rbtree.h>
#include <cpu_types.h>
#include <list_types.h>


/* 协程入口函数原型 */
typedef void *(*cr_function_t)(void *param);

/* 可等待实体，作为协程的等待队列，协程间协作的基础 */
struct cr_waitable_struct {
    struct list_head    wait_queue[1];
};
typedef struct cr_waitable_struct   cr_waitable_t;

/* 协程控制块 */
struct cr_task_struct {
    struct list_head    list_head[1];
    cr_waitable_t       *cur_queue;
    struct {
        int     alive: 1;
        int     ready: 1;
        int     main: 1;
    }   flag;
    
    void    *stack_base;
    size_t  stack_size;
    void    *arg;
    cr_context_t    regs[1];
    cr_function_t   entry;
};
typedef struct cr_task_struct       cr_task_t;

/* 全局协程表 */
struct cr_global_control_table_struct {
    cr_waitable_t       ready_queue[1];
    cr_waitable_t       cancel_queue[1];
    int     task_count;
    int     cancel_count;
    struct {
        int     valid: 1;
    } flag;

    struct cr_task_struct   main_task[1];
    struct cr_task_struct   *current_task;
};
typedef struct cr_global_control_table_struct   cr_gct_t;

/* 通道 */
struct cr_channel_struct {
    cr_waitable_t   waitable[1];

    struct {
        int     opened: 1;
    }   flag;
    
    unsigned int buffer_size;   /* 缓冲区能容纳的项个数 */
    unsigned int pos;   /* 缓冲区的当前位置指针 */
    unsigned int count; /* 缓冲区当前所存放的项个数 */
    void *data[0];
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

#endif /* __CR_TYPES_H__ */
