/* 内存池，通过池化技术对内存分配进行优化 */
#ifndef __CR_POOL_H__
#define __CR_POOL_H__

#include <cr_types.h>


int cr_pool_init(cr_pool_t *pool, size_t pool_size, size_t block_size);
int cr_pool_destroy(cr_pool_t *pool);
int cr_pool_node_alloc(cr_pool_t *pool);
int cr_pool_node_free(cr_pool_node_t *node);
int cr_pool_node_free_all(cr_pool_t *pool);

void *cr_pool_block_alloc(cr_pool_t *pool);
void cr_pool_block_free(cr_pool_t *pool, void *ptr);


#endif /* __CR_POOL_H__ */
