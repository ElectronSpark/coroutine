#include <coroutine.h>
#include <stdio.h>

int count1 = 0;
int count2 = 0;

static void __entry1(void *param) {
    count2 += 1;
    cr_task_exit();
    count2 -= 1;
}

static void __entry(void *param) {
    count1 += 1;
    cr_task_create(__entry1, param);
    cr_task_exit();
    count2 -= 1;
}

int main(void)
{
    printf("hello world!\n");
    cr_init();

    for (int i = 0; i < 1000000; i++) {
        cr_task_create(__entry, (void *)i);
    }

    cr_wait_event_loop();
    printf("count1: %d, count2: %d\n", count1, count2);

    return 0;
}
