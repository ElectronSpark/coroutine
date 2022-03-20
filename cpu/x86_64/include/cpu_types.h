/* x86_64 特定的框架相关数据结构定义 */
#ifndef __CR_CPU_TYPES__
#define __CR_CPU_TYPES__

#include <stddef.h>


/* 通用寄存器的个数 */
#define CR_CPU_GP_REGS          16

/* 协程寄存器上下文 */
struct cr_context_struct {
    void    *gp_regs[CR_CPU_GP_REGS];
};
typedef struct cr_context_struct    cr_context_t;

/* 所有通用寄存器在上下文中的索引 */
#define CR_CPU_INDEX_RAX        0
#define CR_CPU_INDEX_RBX        1
#define CR_CPU_INDEX_RCX        2
#define CR_CPU_INDEX_RDX        3
#define CR_CPU_INDEX_RSI        4
#define CR_CPU_INDEX_RDI        5
#define CR_CPU_INDEX_RSP        6
#define CR_CPU_INDEX_RBP        7
#define CR_CPU_INDEX_R8         8
#define CR_CPU_INDEX_R9         9
#define CR_CPU_INDEX_R10        10
#define CR_CPU_INDEX_R11        11
#define CR_CPU_INDEX_R12        12
#define CR_CPU_INDEX_R13        13
#define CR_CPU_INDEX_R14        14
#define CR_CPU_INDEX_R15        15

/* 栈寄存器在上下文中的索引 */
#define CR_CPU_INDEX_SP         CR_CPU_INDEX_RSP

#endif /* __CR_CPU_TYPES__ */
