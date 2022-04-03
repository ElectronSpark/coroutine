/* 框架基本 API 的头文件 */
#ifndef __ES_COROUTINE_H__
#define __ES_COROUTINE_H__

#include <cr_types.h>
#include <cr_errno.h>
#include <cr_task.h>
#include <cr_waitqueue.h>
#include <cr_channel.h>
#include <cr_semaphore.h>
#include <cr_event.h>


#define CR_TASK_STACK_SIZE          (1UL << 14)
#define CR_DEFAULT_POOL_SIZE        CR_POOL_NODE_SIZE_MAX
#define CR_POOL_TCB_WATERMARK       (CR_DEFAULT_POOL_SIZE / sizeof(cr_task_t))
#define CR_POOL_STACK_WATERMARK     (CR_POOL_NODE_SIZE_MAX / CR_TASK_STACK_SIZE)
#define CR_POOL_EVENT_NODE_WATERMARK    (CR_DEFAULT_POOL_SIZE / sizeof(cr_event_node_t))


int cr_init(void);
int cr_wait_event_loop(void);
int cr_set_idle_handler(cr_function_t handler);


#endif /* __ES_COROUTINE_H__ */
