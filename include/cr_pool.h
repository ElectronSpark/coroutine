/* 内存池，通过池化技术对内存分配进行优化 */
#ifndef __CR_POOL_H__
#define __CR_POOL_H__

#include <cr_types.h>
#include <list.h>


#define CR_POOL_ALIGN_SIZE      sizeof(void*)
#define CR_POOL_NODE_SIZE_MAX   (1UL << 20)
#define CR_POOL_NODE_SIZE_MIN   CR_POOL_ALIGN_SIZE


/* 静态初始化内存池 */
#define CR_POOL_INIT(name, bsize, nsize, wmark) {   \
    .nodes = { LIST_HEAD_INIT((name).nodes[0]) },   \
    .full = { LIST_HEAD_INIT((name).full[0]) }, \
    .partial = { LIST_HEAD_INIT((name).partial[0]) },   \
    .free = { LIST_HEAD_INIT((name).free[0]) },  \
    .full_nodes = 0,    \
    .partial_nodes = 0, \
    .free_nodes = 0,    \
    .block_total = 0,   \
    .block_used = 0,    \
    .block_free = 0,    \
    .block_size = (bsize),  \
    .node_size = (nsize),   \
    .water_mark = (wmark)   \
}

/* 静态声明内存池 */
#define CR_POOL_DECLARE(name, bsize, nsize, wmark)  \
    cr_pool_t name = CR_POOL_INIT(name, bsize, nsize, wmark)

int cr_pool_init(cr_pool_t *pool, size_t pool_size, size_t block_size,
                 int water_mark);
int cr_pool_node_expand(cr_pool_t *pool);
int cr_pool_node_shrink(cr_pool_t *node);
int cr_pool_node_free_all(cr_pool_t *pool);

void *cr_pool_block_alloc(cr_pool_t *pool);
void cr_pool_block_free(cr_pool_t *pool, void *ptr);


#endif /* __CR_POOL_H__ */
