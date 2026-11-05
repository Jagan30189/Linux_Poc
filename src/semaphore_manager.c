#include "semaphore_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>

sem_t *empty_sem;
sem_t *full_sem;
sem_t *log_sem;

int init_semaphores()
{
	empty_sem = sem_open(EMPTY_SEM,
						 O_CREAT,
						 0666,
						 5);

	full_sem = sem_open(FULL_SEM,
						O_CREAT,
						0666,
						0);

	log_sem = sem_open(LOG_SEM,
					   O_CREAT,
					   0666,
					   1);

	if (empty_sem == SEM_FAILED || full_sem == SEM_FAILED || log_sem == SEM_FAILED) {
		return -1;
	}
	return 0;
}

int open_semaphores()
{
	empty_sem = sem_open(EMPTY_SEM, 0);
	full_sem = sem_open(FULL_SEM, 0);
	log_sem = sem_open(LOG_SEM, 0);

	if (empty_sem == SEM_FAILED || full_sem == SEM_FAILED || log_sem == SEM_FAILED) {
		return -1;
	}
	return 0;
}

void cleanup_semaphores()
{
	if (empty_sem != SEM_FAILED && empty_sem != NULL) sem_close(empty_sem);
	if (full_sem != SEM_FAILED && full_sem != NULL) sem_close(full_sem);
	if (log_sem != SEM_FAILED && log_sem != NULL) sem_close(log_sem);

	sem_unlink(EMPTY_SEM);
	sem_unlink(FULL_SEM);
	sem_unlink(LOG_SEM);
}
