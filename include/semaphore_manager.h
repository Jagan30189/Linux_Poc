#ifndef SEMAPHORE_MANAGER_H
#define SEMAPHORE_MANAGER_H

#include <semaphore.h>

#define EMPTY_SEM "/empty_sem"
#define FULL_SEM "/full_sem"
#define LOG_SEM "/log_sem"

extern sem_t *empty_sem;
extern sem_t *full_sem;
extern sem_t *log_sem;

int init_semaphores();

int open_semaphores();

void cleanup_semaphores();

#endif
