#define _POSIX_C_SOURCE 200809L

#include "stats_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define STATS_SHM_NAME "/ecommerce_order_stats"
#define STATS_MUTEX_NAME "/ecommerce_stats_mutex"

static order_stats_t *stats;
static sem_t *stats_mutex;
static int stats_fd = -1;

int stats_open(int reset_if_new)
{
    int created = 0;
    int flags = O_RDWR;

    if (reset_if_new) flags |= O_CREAT | O_EXCL;
    stats_fd = shm_open(STATS_SHM_NAME, flags, 0660);
    if (stats_fd != -1 && reset_if_new) {
        created = 1;
    } else if (stats_fd == -1 && reset_if_new && errno == EEXIST) {
        stats_fd = shm_open(STATS_SHM_NAME, O_RDWR, 0660);
    }
    if (stats_fd == -1) return -1;
    if (ftruncate(stats_fd, sizeof(*stats)) == -1) return -1;

    stats = mmap(NULL, sizeof(*stats), PROT_READ | PROT_WRITE, MAP_SHARED, stats_fd, 0);
    if (stats == MAP_FAILED) { stats = NULL; return -1; }
    stats_mutex = sem_open(STATS_MUTEX_NAME, O_CREAT, 0660, 1);
    if (stats_mutex == SEM_FAILED) { stats_mutex = NULL; return -1; }

    if (created) {
        if (sem_wait(stats_mutex) == -1) return -1;
        memset(stats, 0, sizeof(*stats));
        stats->inventory_remaining = 100;
        sem_post(stats_mutex);
    }
    return 0;
}

void stats_close(void)
{
    if (stats != NULL) munmap(stats, sizeof(*stats));
    if (stats_fd != -1) close(stats_fd);
    if (stats_mutex != NULL) sem_close(stats_mutex);
    stats = NULL;
    stats_mutex = NULL;
    stats_fd = -1;
}

int stats_update(unsigned long produced, unsigned long processed,
                 unsigned long fraud_flags, unsigned long recovered,
                 unsigned long failures, int inventory_delta)
{
    if (stats == NULL || stats_mutex == NULL) return -1;
    if (sem_wait(stats_mutex) == -1) return -1;
    stats->produced += produced;
    stats->processed += processed;
    stats->fraud_flags += fraud_flags;
    stats->recovered += recovered;
    stats->failures += failures;
    stats->inventory_remaining += inventory_delta;
    sem_post(stats_mutex);
    return 0;
}

int stats_read(order_stats_t *snapshot)
{
    if (stats == NULL || stats_mutex == NULL || snapshot == NULL) return -1;
    if (sem_wait(stats_mutex) == -1) return -1;
    *snapshot = *stats;
    sem_post(stats_mutex);
    return 0;
}

void stats_unlink(void)
{
    shm_unlink(STATS_SHM_NAME);
    sem_unlink(STATS_MUTEX_NAME);
}
