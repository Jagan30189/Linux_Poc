#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "order.h"
#include "mq_manager.h"
#include "semaphore_manager.h"
#include "logger.h"

int main(int argc, char **argv)
{
    mqd_t mq;
	int consumer_id = 1;
	int order_count = 10;

	if (argc == 3) {
		consumer_id = atoi(argv[1]);
		order_count = atoi(argv[2]);
	} else if (argc == 2) {
		order_count = atoi(argv[1]);
	} else if (argc != 1) {
		fprintf(stderr, "Usage: %s [consumer-id] [order-count]\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (consumer_id < 1 || order_count < 1) {
		fprintf(stderr, "Consumer ID and order count must be positive.\n");
		return EXIT_FAILURE;
	}

    mq = open_queue();
	if (mq == (mqd_t)-1) {
		perror("mq_open");
		return EXIT_FAILURE;
	}

	if (open_semaphores() == -1) {
		perror("sem_open");
		close_queue(mq);
		return EXIT_FAILURE;
	}

	for (int i = 0; i < order_count; i++)
    {
	 Order order;

	 if (sem_wait(full_sem) == -1) {
			perror("sem_wait");
			break;
		}

	 if (receive_order(mq, &order) == -1) {
			perror("mq_receive");
			sem_post(full_sem);
			break;
		}

	 printf("\nProcessing Order\n");

	 printf("ID       : %d\n",
		 order.order_id);

	 printf("Customer : %s\n",
		 order.customer_name);

	 printf("Amount   : %.2f\n",
		 order.amount);

	 if (log_order(&order) == -1)
            perror("log_order");

	 sem_post(empty_sem);

	 sleep(2);
    }

    close_queue(mq);
	unlink_queue();
	cleanup_semaphores();
	return EXIT_SUCCESS;
}
