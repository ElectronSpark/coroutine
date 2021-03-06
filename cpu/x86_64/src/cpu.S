#define __ASM__
#include <cpu_types.h>


// %rax func(%rdi, %rsi, %rdx, %rcx, %r8, %r9)

// void *__cr_switch_to(void *prev, void *next, void *retval);
.globl __cr_switch_to
__cr_switch_to:
    movq    %rbx,    0x08(%rdi)
    movq    %rcx,    0x10(%rdi)
    movq    %rdx,    0x18(%rdi)
    movq    %rsi,    0x20(%rdi)
    movq    %rdi,    0x28(%rdi)
    movq    %rsp,    0x30(%rdi)
    movq    %rbp,    0x38(%rdi)
    movq    %r8,     0x40(%rdi)
    movq    %r9,     0x48(%rdi)
    movq    %r10,    0x50(%rdi)
    movq    %r11,    0x58(%rdi)
    movq    %r12,    0x60(%rdi)
    movq    %r13,    0x68(%rdi)
    movq    %r14,    0x70(%rdi)
    movq    %r15,    0x78(%rdi)


    movq    %rdx,   %rax
    movq    0x08(%rsi),     %rbx
    movq    0x10(%rsi),     %rcx
    movq    0x18(%rsi),     %rdx
    movq    0x28(%rsi),     %rdi
    movq    0x30(%rsi),     %rsp
    movq    0x38(%rsi),     %rbp
    movq    0x40(%rsi),     %r8
    movq    0x48(%rsi),     %r9
    movq    0x50(%rsi),     %r10
    movq    0x58(%rsi),     %r11
    movq    0x60(%rsi),     %r12
    movq    0x68(%rsi),     %r13
    movq    0x70(%rsi),     %r14
    movq    0x78(%rsi),     %r15

    movq    0x20(%rsi),     %rsi

    ret


// void __cr_save_gp_regs(%rax)
__cr_save_gp_regs:
    movq    %rbx,   0x08(%rax)
    movq    %rcx,   0x10(%rax)
    movq    %rdx,   0x18(%rax)
    movq    %rsi,   0x20(%rax)
    movq    %rdi,   0x28(%rax)
    movq    %rsp,   0x30(%rax)
    movq    %rbp,   0x38(%rax)
    movq    %r8,    0x40(%rax)
    movq    %r9,    0x48(%rax)
    movq    %r10,   0x50(%rax)
    movq    %r11,   0x58(%rax)
    movq    %r12,   0x60(%rax)
    movq    %r13,   0x68(%rax)
    movq    %r14,   0x70(%rax)
    movq    %r15,   0x78(%rax)
    retq


// void __cr_context_init(void *arg1, void *arg2,
//                        void *arg3, void *stack_end,
//                        void *func_entry, void *regs);
.globl __cr_context_init
__cr_context_init:
    pushq   %rbp
    movq    %rsp,   %rbp

    movq    %rcx,   %rsp
    movq    %r9,    %rax
    subq    $8,     %rsp
    callq   __cr_save_gp_regs
    pushq   %r8
    movq    %rsp,   0x30(%rax)
    xor     %rax,   %rax

    movq    %rbp,   %rsp
    popq    %rbp
    ret
