/* x86_64 特定的框架相关接口定义 */
#ifndef __CR_CPU_SPECIFIC_H__
#define __CR_CPU_SPECIFIC_H__

#include <cpu_types.h>

/* 下面几个函数用汇编实现 */
extern cr_context_t *__cr_switch_to(void *prev, void *next,
                                    cr_context_t *last);
extern void __cr_context_init(void *arg1, void *arg2,
                              void *stack_end, void *func_entry);
extern void *__cr_context_save(void *regs);

/* 上下文切换的 cpu 特定实现 */
static inline cr_context_t *cr_switch_to(cr_context_t *prev,
                                         cr_context_t *next)
{
    if (!prev || !next) {
        return NULL;
    }

    return __cr_switch_to(prev->gp_regs, next->gp_regs, prev);
}

/* 初始化协程上下文 */
static inline void cr_context_init(
    cr_context_t *self,
    void *stack,
    size_t stack_size,
    void *func_entry,
    void *arg1,
    void *arg2
)
{
    /* 如果上一个协程是主协程，说明当前协程是新创建的协程 */
    if (__cr_context_save(self->gp_regs) == self->gp_regs) {
        __cr_context_init(arg1, arg2, stack + stack_size, func_entry);
    }
}

#endif /* __CR_CPU_SPECIFIC_H__ */
