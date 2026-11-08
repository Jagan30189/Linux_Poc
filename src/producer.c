#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "order.h"
#include "mq_manager.h"
#include "semaphore_manager.h"

int main(int argc, char **argv)
{
	mqd_t mq;
	int producer_id = 1;
	int order_count = 10;

	if (argc == 3) {
		producer_id = atoi(argv[1]);
		order_count = atoi(argv[2]);
	} else if (argc != 1) {
		fprintf(stderr, "Usage: %s [producer-id order-count]\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (producer_id < 1 || order_count < 1) {
		fprintf(stderr, "Producer ID and order count must be positive.\n");
		return EXIT_FAILURE;
	}

	mq = create_queue();
	if (mq == (mqd_t)-1) {
		perror("mq_open");
		return EXIT_FAILURE;
	}

	if (init_semaphores() == -1) {
		perror("sem_open");
		close_queue(mq);
		return EXIT_FAILURE;
	}

	for (int i = 1; i <= order_count; i++)
	{
		Order order = {};

		order.order_id = producer_id * 1000 + i;

		snprintf(order.customer_name, sizeof(order.customer_name),
				 "Customer_%d", order.order_id);

		order.amount = i * 100.0;

		order.priority = (i % 3) + 1;

		if (sem_wait(empty_sem) == -1) {
			perror("sem_wait");
			break;
		}

		if (send_order(mq, &order) == -1) {
			perror("mq_send");
			sem_post(empty_sem);
			break;
		}

		printf("Produced Order %d\n",
			   order.order_id);

		sem_post(full_sem);

		sleep(1);
	}

	close_queue(mq);
	return EXIT_SUCCESS;
}
