#include <coroutine.h>
#include <stdio.h>
#include <sys/time.h>

int count1 = 0;
int count2 = 0;

static void __entry(void *param) {
    count1 += 1;
    cr_yield();
    count2 += 1;
}

int main(int argc, char *argv[])
{
    int task_num = atoi(argv[1]);
    double ctime = 0;
    double rtime = 0;
    struct timeval tv[3] = { 0 };

    cr_init();    

    gettimeofday(&tv[0], NULL);
    for (long i = 0; i < task_num; i++) {
        cr_task_create(__entry, NULL);
    }

    gettimeofday(&tv[1], NULL);
    cr_wait_event_loop();
    gettimeofday(&tv[2], NULL);
    
    ctime = ((double)tv[1].tv_sec - (double)tv[0].tv_sec) * 1000 + \
            ((double)tv[1].tv_usec - (double)tv[0].tv_usec) / 1000;
    rtime = ((double)tv[2].tv_sec - (double)tv[1].tv_sec) * 1000 + \
            ((double)tv[2].tv_usec - (double)tv[1].tv_usec) / 1000;
    
    if (count1 != count2 || count1 != task_num) {
        printf("ERROR!\n");
    } else {
        // printf("task_num: %d\ttime of create: %lf(us)\ttime of run: %lf(us)\n",
        //        task_num, ctime, rtime);
        printf("%d,%lf,%lf\n", task_num, ctime, rtime);
    }

    return 0;
}
