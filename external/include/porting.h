/******************************************************************************
 * @file porting.h
 * @author ElectronSpark (820461770@qq.com)
 * @brief 用于移植linux内核某些数据结构的头文件
 * @version 0.1
 * @date 2021-01-17
 * 
 * @copyright Copyright (c) 2021
 * 
 *****************************************************************************/
#ifndef _PORTING_H_
#define _PORTING_H_

#include <stdint.h>
#include <stdbool.h>

#include "poison.h"

#define unlikely	__glibc_unlikely
#define likely	__glibc_unlikely
#define __rcu
#define __force

#define GOLDEN_RATIO_32 0x61C88647
#define GOLDEN_RATIO_64 0x61C8864680B583EBull

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	((type *)(__mptr - offsetof(type, member))); })


/*******************    FROM "linux/compiler.h"    ********************/
#define READ_ONCE(x)	(x)

#define WRITE_ONCE(x, val)						\
do {									\
	x = (val);						\
} while (0)

/*******************    FROM "linux/barrier.h"    ********************/

static inline void barrier(void)
{
	asm volatile("" : : : "memory");
}

/*******************    FROM "linux/rcupdate.h"    ********************/
#define rcu_assign_pointer(p, v)					      \
do {									      \
	p = (v);				      \
} while (0)


#endif
