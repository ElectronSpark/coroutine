#include <coroutine.h>
#include <cr_event.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>


#define CR_EVENT_GROUP_SIZE     16
#define CR_TOP_TASK_NUM         10


int count1 = 0;
int count2 = 0;
int count3 = 0;

static CR_EVENT_DECLARE(__sevent);
static CR_EVENT_DECLARE(__alarm);


static void __trd_entry(void *param)
{
    cr_eid_t eid = (cr_eid_t)param;
    void *ret_data = (void *)0xfdfdfdfd;

    if (cr_event_wait(&__sevent, eid, param, &ret_data) != 0) {
        return;
    }
    if (ret_data == param) {
        count3 += 1;
    }
}

static void __sec_entry(void *param)
{
    cr_eid_t eid = (cr_eid_t)param;
    void *ret_data = (void *)0xfdfdfdfd;

    if (cr_event_wait(&__sevent, eid, param, &ret_data) != 0) {
        return;
    }
    if (ret_data != param) {
        return;
    }
    cr_yield();
    
    eid += 1;
    if (cr_event_get_data(&__sevent, eid, &ret_data) != 0) {
        return;
    }
    if ((cr_eid_t)ret_data != eid) {
        return;
    }
    if (cr_event_notify(&__sevent, eid, (void *)eid) == 0) {
        count2 += 1;
    }
}

static void __top_entry(void *param)
{
    cr_eid_t eid = (cr_eid_t)param;
    void *ret_data = (void *)0xfdfdfdfd;

    if (cr_event_wait(&__alarm, eid, param, &ret_data) != 0) {
        return;
    }
    if (ret_data != param) {
        return;
    }
    for (long i = 0; i < CR_EVENT_GROUP_SIZE; i++) {
        cr_eid_t j = (eid + 1) * CR_EVENT_GROUP_SIZE * 2 + i;
        if (cr_event_get_data(&__sevent, j, &ret_data) != 0) {
            continue;
        }
        if ((cr_eid_t)ret_data != j) {
            continue;
        }
        if (cr_event_notify(&__sevent, j, (void *)j) == 0) {
            count1 += 1;
        }
    }
}

static void __sig_exit(int param)
{
    printf("overtime!\n");
    exit(0);
}

static void __sig_handler(int param)
{
    void *ret_data = (void *)0xfdfdfdfd;

    printf("alarm\n");
    for (long i = 0; i < CR_TOP_TASK_NUM; i++) {
        if (cr_event_get_data(&__alarm, (cr_eid_t)i, &ret_data) != 0) {
            continue;
        }
        if ((long)ret_data != i) {
            continue;
        }
        cr_event_notify(&__alarm, (cr_eid_t)i, (void *)i);
    }
    printf("done\n");
    signal(SIGALRM, __sig_exit);
    alarm(5);
}


int main(void)
{
    cr_init();

    signal(SIGALRM, __sig_handler);
    alarm(5);
    printf("creating tasks\n");
    for (long i = 0; i < CR_TOP_TASK_NUM; i++) {
        cr_task_create(__top_entry, (void *)i);
        for (long j = 0; j < CR_EVENT_GROUP_SIZE; j++) {
            long param = (i + 1) * CR_EVENT_GROUP_SIZE * 2 + j;
            cr_task_create(__sec_entry, (void *)param);
            cr_task_create(__trd_entry, (void *)(param + 1));
        }
    }

    printf("wait for 5 seconds...");

    cr_wait_event_loop();
    return 0;
}
