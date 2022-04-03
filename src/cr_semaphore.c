#include <coroutine.h>
#include <cr_task.h>
#include <cr_waitqueue.h>
#include <cr_semaphore.h>


/* 检查处于给定状态的信号量是否合法 */
#define __check_sem_attr_valid(limit, value)    \
    ((value) >= 0 && (limit) > 0 && (value) <= (limit))

/* 查看信号量是否打开 */
#define __check_sem_flag_open(sem)   ((sem)->flag.opened)

/* 查看信号量的有效标志 */
#define __check_sem_flag_valid(sem)  ((sem)->flag.valid)

/* 检查信号量是否有效，返回布尔值 */
#define __check_sem_valid(sem)  \
    (!!(sem) && \
    __check_sem_attr_valid((sem)->limit, (sem)->value) && \
    __check_sem_flag_valid(sem))

/* 检查信号量是否有效且处于打开状态，返回布尔值 */
#define __check_sem_valid_open(sem) \
    (__check_sem_valid(sem) && __check_sem_flag_open(sem))


/* 初始化一个信号量 */
int cr_sem_init(cr_sem_t *sem, int limit, int value)
{
    int ret = -1;

    if (!sem || !__check_sem_attr_valid(limit, value)) {
        return -1;
    }

    if ((ret = cr_waitqueue_init(sem->waitqueue)) != 0) {
        return -1;
    }

    sem->limit = limit;
    sem->value = value;
    sem->flag.valid = 1;
    sem->flag.opened = 1;
    return 0;
}

/* 关闭一个信号量，并唤醒正在等待该信号量的所有协程 */
int cr_sem_close(cr_sem_t *sem)
{
    if (!__check_sem_valid_open(sem)) {
        return -1;
    }

    sem->flag.opened = 0;
    return cr_waitqueue_notify_all(sem->waitqueue);
}

/* 尝试获得一个信号量 */
int cr_sem_wait(cr_sem_t *sem)
{
    if (!__check_sem_valid_open(sem)) {
        return -1;
    }

    if (sem->value > 0) {
        sem->value -= 1;
        return 0;
    } 

    if (cr_await(sem->waitqueue) != 0) {
        return -1;
    }

    return __check_sem_flag_open(sem) ? 0 : -1;
}

/* 释放一个信号量 */
int cr_sem_post(cr_sem_t *sem)
{
    if (!__check_sem_valid(sem)) {
        return -1;
    }

    if (!cr_is_waitqueue_empty(sem->waitqueue)) {
        return cr_waitqueue_notify(sem->waitqueue);
    }

    if (sem->value < sem->limit) {
        sem->value += 1;
        return 0;
    }
    return -1;
}

/* 获得信号量的值 */
int cr_sem_getvalue(cr_sem_t *sem)
{
    if (!__check_sem_valid(sem)) {
        return -1;
    }

    return sem->value;
}
