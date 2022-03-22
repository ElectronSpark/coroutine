/* 框架基本 API 的头文件 */
#ifndef __ES_COROUTINE_H__
#define __ES_COROUTINE_H__

#include <cr_types.h>
#include <cr_task.h>
#include <cr_waitable.h>
#include <cr_channel.h>


int cr_init(void);
int cr_wait_event_loop(void);


#endif /* __ES_COROUTINE_H__ */
