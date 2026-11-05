#include <stdio.h>
#include "logger.h"
#include "semaphore_manager.h"

int log_order(const Order *order)
{
	FILE *fp;

	if (sem_wait(log_sem) == -1)
		return -1;

	fp = fopen("logs/orders.log", "a");

	if (fp == NULL) {
		sem_post(log_sem);
		return -1;
	}

	int result = fprintf(fp,
						 "OrderID=%d Customer=%s Amount=%.2f Priority=%d\n",
						 order->order_id,
						 order->customer_name,
						 order->amount,
						 order->priority) < 0 ? -1 : 0;

	fclose(fp);
	sem_post(log_sem);
	return result;
}
