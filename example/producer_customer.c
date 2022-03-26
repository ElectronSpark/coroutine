#include <coroutine.h>
#include <stdio.h>


#define CR_PRODUCT_TOTAL    10000
#define CR_CONSUMERS        100
#define CR_PRODUCERS        10


cr_sem_t producer_sem[1];
cr_sem_t consumer_sem[1];

int consume_count = 0;
int produce_count = 0;
int product_valid = 0;


static void __comsumer_entry(void *param)
{
    while (1) {
        if (cr_sem_wait(consumer_sem) != 0) {
            break;
        }
        if (product_valid == 1) {
            consume_count += 1;
            product_valid = 0;
        }
        if (cr_sem_post(producer_sem) != 0) {
            break;
        }
    }
}

static void __producer_entry(void *param)
{
    static int cnt = 0;
    while (1) {
        if (cr_sem_wait(producer_sem) != 0) {
            break;
        }
        if (cnt >= CR_PRODUCT_TOTAL) {
            cr_sem_close(consumer_sem);
            cr_sem_close(producer_sem);
            break;
        }
        if (product_valid == 0) {
            produce_count += 1;
            product_valid = 1;
        }
        cnt += 1;
        if (cr_sem_post(consumer_sem) != 0) {
            break;
        }
    }
}


int main(void)
{
    cr_init();
    cr_sem_init(producer_sem, 1, 1);
    cr_sem_init(consumer_sem, 1, 0);

    for (int i = 0; i < CR_CONSUMERS; i++) {
        cr_task_create(__comsumer_entry, NULL);
    }

    for (int i = 0; i < CR_PRODUCERS; i++) {
        cr_task_create(__producer_entry, NULL);
    }

    cr_wait_event_loop();

    printf("products: %d, producers: %d, consumers: %d\n",
           CR_PRODUCT_TOTAL, CR_PRODUCERS, CR_CONSUMERS);
    printf("consume_count: %d, produce_count: %d\n",
           consume_count, produce_count);

    return 0;
}
