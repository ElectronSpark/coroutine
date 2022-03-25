#include <coroutine.h>
#include <stdio.h>

static void *__entry(void *param) {
    printf("task: %d\n", (int)param);
    return NULL;
}

int main(void)
{
    printf("hello world!\n");
    cr_init();

    for (int i = 0; i < 10; i++) {
        cr_task_create(__entry, (void *)i);
    }

    cr_wait_event_loop();

    return 0;
}
