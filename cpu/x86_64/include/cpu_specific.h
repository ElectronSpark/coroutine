/* x86_64 特定的框架相关接口定义 */
#ifndef __CR_CPU_SPECIFIC_H__
#define __CR_CPU_SPECIFIC_H__

#include <string.h>
#include <cpu_types.h>


/* 下面几个函数用汇编实现 */
extern void *__cr_switch_to(void *prev, void *next, void *retval);
extern void __cr_context_init(void *arg1, void *arg2,
                              void *arg3, void *stack_end,
                              void *func_entry, void *regs);

/* 上下文切换的 cpu 特定实现 */
static inline void *cr_switch_to(cr_context_t *prev,
                                 cr_context_t *next,
                                 void *retval)
{
    if (!prev || !next) {
        return NULL;
    }

    return __cr_switch_to(prev->gp_regs, next->gp_regs, retval);
}

/* 初始化协程上下文 */
static inline void cr_context_init(
    cr_context_t *target,
    void *stack,
    size_t stack_size,
    void *func_entry,
    void *arg1,
    void *arg2,
    void *arg3
)
{
    __cr_context_init(arg1, arg2, arg3, stack + stack_size,
                      func_entry, target->gp_regs);
}

#endif /* __CR_CPU_SPECIFIC_H__ */
