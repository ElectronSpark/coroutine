#include <malloc.h>
#include <coroutine.h>
#include <cr_pool.h>


/* 初始化内存池 */
int cr_pool_init(cr_pool_t *pool, size_t pool_size, size_t block_size,
                 int water_mark)
{
    if (!pool) {
        return -1;
    }

    if (water_mark < 0) {
        return -1;
    }

    /* 内存池节点大小限制 */
    if (pool_size < block_size || pool_size > CR_POOL_NODE_SIZE_MAX) {
        return -1;
    }

    INIT_LIST_HEAD(pool->nodes);
    INIT_LIST_HEAD(pool->full);
    INIT_LIST_HEAD(pool->partial);
    INIT_LIST_HEAD(pool->free);
    pool->full_nodes = 0;
    pool->partial_nodes = 0;
    pool->free_nodes = 0;
    pool->block_total = 0;
    pool->block_used = 0;
    pool->block_free = 0;
    pool->block_size = block_size;
    pool->node_size = pool_size;
    pool->water_mark = water_mark;

    return 0;
}

/* 利用 malloc 分配新的内存池节点 */
static cr_pool_node_t *__node_alloc(size_t block_size, size_t buffer_size)
{
    block_size = (block_size + CR_POOL_ALIGN_SIZE - 1) & ~(CR_POOL_ALIGN_SIZE - 1);
    buffer_size = (buffer_size + CR_POOL_ALIGN_SIZE - 1) & ~(CR_POOL_ALIGN_SIZE - 1);

    if (block_size <= 0 || buffer_size < block_size) {
        return NULL;
    }

    /* 为内存池节点的结构体分配内存 */
    cr_pool_node_t *new_node = malloc(sizeof(cr_pool_node_t));
    if (!new_node) {
        return NULL;
    }

    /* 为内存池节点的缓存区分配内存 */
    unsigned int *new_buffer = malloc(buffer_size);
    if (!new_buffer) {
        free(new_node);
        return NULL;
    }

    /* 计算一个节点可以容纳多少个内存池对象 */
    int blocks_num = buffer_size / block_size;

    /* 开始进行初始化 */
    new_node->block_size = block_size;
    new_node->node_size = buffer_size;
    new_node->block_free = blocks_num;
    new_node->block_total = blocks_num;
    new_node->block_used = 0;
    new_node->pool = NULL;
    new_node->buffer = new_buffer;
    new_node->first_free = new_buffer;
    INIT_LIST_HEAD(new_node->list_head);
    INIT_LIST_HEAD(new_node->index_head);

    /* 将所有内存池对象穿成一个链表 */
    void *last_ptr = NULL;
    void *next_ptr = new_buffer;
    for (int i = 1; i < blocks_num; i++) {
        last_ptr = next_ptr;
        next_ptr += block_size;
        *(void **)last_ptr = next_ptr;
    }

    *(void **)last_ptr = NULL;

    return new_node;
}

/* 将一个内存池节点加到加入内存池 */
static int __node_attach(cr_pool_t *pool, cr_pool_node_t *node)
{
    struct list_head *pos = NULL;
    cr_pool_node_t *pos_entry = NULL;

    if (!pool || !node) {
        return -1;
    }

    if (pool->block_size != node->block_size ||
        pool->node_size != node->node_size)
    {
        return -1;
    }

    /* 加入到用于定位内存池节点的链表中，需要进行排序操作 */
    node->pool = pool;
    /* 如果当前内存池没有节点，直接插入 */
    if (list_empty(pool->nodes)) {
        pos = pool->nodes;
    } else {
        /* 找到合适的后继节点 */
        list_for_each(pos, pool->nodes) {
            pos_entry = list_entry(pos, cr_pool_node_t, index_head[0]);
            if (pos_entry->buffer >= node->buffer + node->node_size) {
                break;
            }
        }
    }
    /* 插入到找到的节点之前 */
    list_add_tail(node->index_head, pos);
    
    /* 加入到对应的链表中 */
    if (node->block_used == 0) {
        list_add(pool->free, node->list_head);
        pool->free_nodes += 1;
    } else if (node->block_used < node->block_total) {
        list_add(pool->partial, node->list_head);
        pool->partial_nodes += 1;
    } else {
        list_add(pool->full, node->list_head);
        pool->full_nodes += 1;
    }

    pool->block_total += node->block_total;
    pool->block_free += node->block_free;
    pool->block_used += node->block_used;

    return 0;
}

/* 将一个内存池节点从其所在内存池中剥离 */
static int __node_detach(cr_pool_node_t *node)
{
    if (!node) {
        return -1;
    }

    cr_pool_t *pool = node->pool;
    if (!pool) {
        return -1;
    }

    if (node->block_used == 0) {
        pool->free_nodes -= 1;
    } else if (node->block_used < node->block_total) {
        pool->partial_nodes -= 1;
    } else {
        pool->full_nodes -= 1;
    }

    list_del_init(node->list_head);
    list_del_init(node->index_head);
    pool->block_total -= node->block_total;
    pool->block_free -= node->block_free;
    pool->block_used -= node->block_used;

    return 0;
}

/* 释放一个利用 malloc 分配的内存池节点，被操作的节点需要先 detach 掉 */
static void __node_free(cr_pool_node_t *node)
{
    if (!node) {
        if (node->buffer) {
            free(node->buffer);
        }
        free(node);
    }
}

/* 查找给定地址所在的内存池节点 */
static cr_pool_node_t *__find_pool_node(cr_pool_t *pool, void *ptr)
{
    struct list_head *pos = NULL;
    cr_pool_node_t *pos_entry = NULL;
    list_for_each(pos, pool->nodes) {
        pos_entry = list_entry(pos, cr_pool_node_t, index_head[0]);
        if (pos_entry->buffer <= ptr &&
            ptr < pos_entry->buffer + pos_entry->node_size
        ) {
            break;
        }
        pos_entry = NULL;
    }

    return pos_entry;
}

/* 将内存池对象放入内存池节点中 */
static int __push_node_block(cr_pool_node_t *node, void *ptr)
{
    int idx = 0;
    void **block_base = NULL;

    if (!node || !ptr) {
        return -1;
    }

    if (ptr < node->buffer || ptr >= node->buffer + node->node_size) {
        return -1;
    }

    idx = (ptr - node->buffer) / node->block_size;
    block_base = node->buffer + idx * node->block_size;
    *block_base = node->first_free;
    node->first_free = (void *)block_base;

    node->block_free += 1;
    node->block_used -= 1;

    return 0;
}

/* 从内存池节点中弹出一个内存池对象 */
static void *__pop_node_block(cr_pool_node_t *node)
{
    void *ptr = NULL;

    if (!node) {
        return NULL;
    }

    if ((ptr = node->first_free) == NULL) {
        return NULL;
    }

    node->first_free = *(void **)ptr;
    node->block_free -= 1;
    node->block_used += 1;

    return ptr;
}

/* 分配一个新的内存池节点 */
int cr_pool_node_expand(cr_pool_t *pool)
{
    int ret = -1;

    if (!pool) {
        return -1;
    }

    cr_pool_node_t *new_node = __node_alloc(pool->block_size, pool->node_size);
    if (!new_node) {
        return -1;
    }

    if ((ret = __node_attach(pool, new_node)) != 0) {
        return ret;
    }

    return 0;
}

/* 释放空闲的内存池节点，直到空闲的内存池实例的数量低于 watermark */
int cr_pool_node_shrink(cr_pool_t *pool)
{
    int free_count = 0;
    cr_pool_node_t *node = NULL;

    if (!pool) {
        return free_count;
    }

    while (pool->block_free > pool->water_mark && !list_empty(pool->free)) {
        node = list_first_entry(pool->free, cr_pool_node_t, index_head[0]);
        if (__node_detach(node) != 0) {
            break;
        }
        __node_free(node);
        free_count++;
    }

    return free_count;
}

/* 强行释放所有的内存池节点 */
int cr_pool_node_free_all(cr_pool_t *pool)
{
    cr_pool_node_t *pos = NULL;
    cr_pool_node_t *n = NULL;

    if (!pool) {
        return -1;
    }

    list_for_each_entry_safe(pos, n, pool->nodes, index_head[0]) {
        __node_detach(pos);
        __node_free(pos);
    }

    return cr_pool_init(pool, pool->node_size, pool->block_size,
                        pool->water_mark);
}

/* 调整内存池节点所在的类型 */
static void __adjust_node_list(cr_pool_node_t *node, int origin_used)
{
    cr_pool_t *pool = NULL;

    if (!node || (pool = node->pool) == NULL) {
        return;
    }


    list_del_init(node->list_head);
    if (origin_used == 0) {
        pool->free_nodes -= 1;
    } else if (origin_used < pool->node_size / pool->block_size) {
        pool->partial_nodes -= 1;
    } else {
        pool->full_nodes -= 1;
    }


    if (node->block_used == 0) {
        pool->free_nodes += 1;
        list_add(node->list_head, pool->free);
    } else if (node->block_used < pool->node_size / pool->block_size) {
        pool->partial_nodes += 1;
        list_add(node->list_head, pool->partial);
    } else {
        pool->full_nodes += 1;
        list_add(node->list_head, pool->full);
    }
}

/* 从内存池中分配一个内存块 */
void *cr_pool_block_alloc(cr_pool_t *pool)
{
    void *ret = NULL;
    int origin_used = 0;
    cr_pool_node_t *node = NULL;

    if (!pool) {
        return NULL;
    }

    if (!list_empty(pool->partial)) {
        node = list_first_entry(pool->partial, cr_pool_node_t, list_head[0]);
    } else if (!list_empty(pool->free)) {
         node = list_first_entry(pool->free, cr_pool_node_t, list_head[0]);
    } else {
        node = __node_alloc(pool->block_size, pool->node_size);
        if (!node) {
            return NULL;
        }
        if (__node_attach(pool, node) != 0) {
            __node_free(node);
            return NULL;
        }
    }

    origin_used = node->block_used;
    ret = __pop_node_block(node);
    __adjust_node_list(node, origin_used);

    return ret;
}

/* 将内存块归还到一个内存池中 */
void cr_pool_block_free(cr_pool_t *pool, void *ptr)
{
    int origin_used = 0;
    cr_pool_node_t *node = NULL;

    if (!pool || !ptr) {
        return;
    }

    node = __find_pool_node(pool, ptr);
    if (node) {
        origin_used = node->block_used;
        __push_node_block(node, ptr);
        __adjust_node_list(node, origin_used);
    }
}
