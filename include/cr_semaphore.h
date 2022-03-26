#ifndef __CR_SEMAPHORE_H__
#define __CR_SEMAPHORE_H__

#include <cr_types.h>


int cr_sem_init(cr_sem_t *sem, int value, int limit);
int cr_sem_close(cr_sem_t *sem);
int cr_sem_wait(cr_sem_t *sem);
int cr_sem_post(cr_sem_t *sem);
int cr_sem_getvalue(cr_sem_t *sem);


#endif /* __CR_SEMAPHORE_H__ */
