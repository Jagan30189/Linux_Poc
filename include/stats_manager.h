#ifndef STATS_MANAGER_H
#define STATS_MANAGER_H

#include <stddef.h>

typedef struct {
    unsigned long produced;
    unsigned long processed;
    unsigned long fraud_flags;
    unsigned long recovered;
    unsigned long failures;
    int inventory_remaining;
} order_stats_t;

int stats_open(int reset_if_new);
void stats_close(void);
int stats_update(unsigned long produced, unsigned long processed,
                 unsigned long fraud_flags, unsigned long recovered,
                 unsigned long failures, int inventory_delta);
int stats_read(order_stats_t *snapshot);
void stats_unlink(void);

#endif
