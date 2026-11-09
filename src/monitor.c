#include "mq_manager.h"
#include "stats_manager.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    mqd_t mq = open_queue();
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return EXIT_FAILURE;
    }
    if (stats_open(0) == -1) {
        perror("stats_open");
        close_queue(mq);
        return EXIT_FAILURE;
    }

    struct mq_attr attributes;
    order_stats_t snapshot;
    if (mq_getattr(mq, &attributes) == -1 || stats_read(&snapshot) == -1) {
        perror("monitor");
        stats_close();
        close_queue(mq);
        return EXIT_FAILURE;
    }

    printf("Queue: %ld/%ld messages\n", attributes.mq_curmsgs, attributes.mq_maxmsg);
    printf("Produced: %lu\nProcessed: %lu\nFraud flags: %lu\n",
           snapshot.produced, snapshot.processed, snapshot.fraud_flags);
    printf("Recovered: %lu\nFailures: %lu\nInventory remaining: %d\n",
           snapshot.recovered, snapshot.failures, snapshot.inventory_remaining);

    stats_close();
    close_queue(mq);
    return EXIT_SUCCESS;
}
