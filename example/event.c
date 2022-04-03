#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <coroutine.h>
#include <cr_event.h>

#define stub()  printf("%s()-%d\n", __func__, __LINE__)
// #define stub()

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
    cr_event_node_t *enode = NULL;
    void *ret_data = (void *)0xfdfdfdfd;

    if ((enode = cr_event_add(&__sevent, eid)) == NULL) {
        stub();
        return;
    }
    if (cr_event_wait(enode, param, NULL) != 0) {
        cr_event_remove(enode, cr_ptr2err(CR_ERR_OK));
        stub();
        return;
    }
    if (cr_event_get_data(enode, &ret_data) == 0 && ret_data == param) {
        count3 += 1;
    }
    cr_event_remove(enode, cr_ptr2err(CR_ERR_OK));
}

static void __sec_entry(void *param)
{
    cr_eid_t eid = (cr_eid_t)param;
    cr_event_node_t *enode = NULL;
    void *ret_data = (void *)0xfdfdfdfd;

    if ((enode = cr_event_add(&__sevent, eid)) == NULL) {
        stub();
        return;
    }
    if (cr_event_wait(enode, param, NULL) != 0) {
        cr_event_remove(enode, cr_ptr2err(CR_ERR_OK));
        stub();
        return;
    }
    if (cr_event_get_data(enode, &ret_data) != 0 || ret_data != param) {
        cr_event_remove(enode, cr_ptr2err(CR_ERR_OK));
        stub();
        return;
    }
    cr_event_remove(enode, cr_ptr2err(CR_ERR_OK));
    cr_sched(NULL);
    
    eid += 1;
    if ((enode = cr_event_find(&__sevent, eid)) == NULL) {
        stub();
        return;
    }
    if (cr_event_get_data(enode, &ret_data) != 0) {
        stub();
        return;
    }
    if ((cr_eid_t)ret_data != eid) {
        stub();
        return;
    }
    if (cr_event_notify(enode, (void *)eid) == 0) {
        count2 += 1;
    }
}

static void __top_entry(void *param)
{
    cr_eid_t eid = (cr_eid_t)param;
    cr_event_node_t *enode = NULL;
    void *ret_data = (void *)0xfdfdfdfd;

    if ((enode = cr_event_add(&__alarm, eid)) == NULL) {
        stub();
        return;
    }
    if (cr_event_wait(enode, param, cr_ptr2err(CR_ERR_OK)) != 0) {
        cr_event_remove(enode, cr_ptr2err(CR_ERR_OK));
        stub();
        return;
    }
    if (cr_event_get_data(enode, &ret_data) != 0 && ret_data != param) {
        cr_event_remove(enode, cr_ptr2err(CR_ERR_OK));
        stub();
        return;
    }
    cr_event_remove(enode, cr_ptr2err(CR_ERR_OK));


    for (long i = 0; i < CR_EVENT_GROUP_SIZE; i++) {
        cr_eid_t j = (eid + 1) * CR_EVENT_GROUP_SIZE + i;
        j *= 2;
        if ((enode = cr_event_find(&__sevent, j)) == NULL) {
            stub();
            continue;
        }
        if (cr_event_get_data(enode, &ret_data) != 0) {
            stub();
            continue;
        }
        if ((cr_eid_t)ret_data != j) {
            stub();
            continue;
        }
        if (cr_event_notify(enode, (void *)j) == 0) {
            count1 += 1;
        }
    }
}

static void __sig_exit(int param)
{
    printf("overtime!\n");
    printf("CR_EVENT_GROUP_SIZE: %d, CR_TOP_TASK_NUM: %d\n",
           CR_EVENT_GROUP_SIZE, CR_TOP_TASK_NUM);
    printf("count1: %d, count2: %d, count3: %d\n", count1, count2, count3);
    exit(0);
}

static void __sig_handler(int param)
{
    static int flag = 0;
    cr_event_node_t *enode = NULL;
    void *ret_data = (void *)0xfdfdfdfd;

    if (flag++ == 0) {
        printf("alarm\n");
        for (long i = 0; i < CR_TOP_TASK_NUM; i++) {
            if ((enode = cr_event_find(&__alarm, (cr_eid_t)i)) == NULL) {
                stub();
                continue;
            }
            if (cr_event_get_data(enode, &ret_data) != 0) {
                stub();
                continue;
            }
            if ((long)ret_data != i) {
                stub();
                continue;
            }
            cr_event_notify(enode, (void *)i);
        }
        printf("done\n");
    } else {
        printf("overtime!\n");
        printf("CR_EVENT_GROUP_SIZE: %d, CR_TOP_TASK_NUM: %d\n",
            CR_EVENT_GROUP_SIZE, CR_TOP_TASK_NUM);
        printf("count1: %d, count2: %d, count3: %d\n", count1, count2, count3);
        exit(0);
    }
}


int main(void)
{
    struct sigaction act = { 0 };
    cr_init();

    act.sa_handler = __sig_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    alarm(0);
    sigaction(SIGALRM, &act, NULL);

    printf("creating tasks...\n");
    fflush(stdout);
    for (long i = 0; i < CR_TOP_TASK_NUM; i++) {
        cr_task_create(__top_entry, (void *)i);
        for (long j = 0; j < CR_EVENT_GROUP_SIZE; j++) {
            long param = (i + 1) * CR_EVENT_GROUP_SIZE + j;
            param *= 2;
            cr_task_create(__sec_entry, (void *)param);
            cr_task_create(__trd_entry, (void *)(param + 1));
        }
    }
    printf("wait for 2 seconds...");
    fflush(stdout);
    alarm(2);

    cr_wait_event_loop();
    printf("CR_EVENT_GROUP_SIZE: %d, CR_TOP_TASK_NUM: %d\n",
           CR_EVENT_GROUP_SIZE, CR_TOP_TASK_NUM);
    printf("count1: %d, count2: %d, count3: %d\n", count1, count2, count3);
    return 0;
}
