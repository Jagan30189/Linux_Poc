#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "order.h"
#include "mq_manager.h"
#include "semaphore_manager.h"
#include "logger.h"
#include "stats_manager.h"

static int fraud_detected(const Order *order)
{
	return order->priority >= 3 && order->amount >= 500.0;
}

static int valid_order(const Order *order)
{
	return order->order_id > 0 && order->customer_name[0] != '\0' &&
		   order->product_id > 0 && order->quantity > 0 &&
		   order->amount > 0.0 && order->priority > 0 && order->priority <= 3;
}

int main(int argc, char **argv)
{
    mqd_t mq;
	int consumer_id = 1;
	int order_count = 10;
	int exit_status = EXIT_SUCCESS;

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
	if (stats_open(0) == -1) {
		perror("stats_open");
		close_queue(mq);
		return EXIT_FAILURE;
	}

	for (int i = 0; i < order_count; i++)
    {
	 Order order;

	 if (sem_wait(full_sem) == -1) {
			perror("sem_wait");
			exit_status = EXIT_FAILURE;
			break;
		}

	 if (receive_order(mq, &order) == -1) {
			perror("mq_receive");
			sem_post(full_sem);
			exit_status = EXIT_FAILURE;
			break;
		}

		if (!valid_order(&order)) {
			stats_update(0, 0, 0, 0, 1, 0);
			fprintf(stderr, "Rejected invalid order\n");
			sem_post(empty_sem);
			continue;
		}

	 int fraud = fraud_detected(&order);
	 stats_update(0, 1, fraud, 0, 0, -order.quantity);

	 printf("\nProcessing Order\n");

	 printf("ID       : %d\n",
		 order.order_id);

	 printf("Customer : %s\n",
		 order.customer_name);

	 printf("Product  : %d\nQuantity : %d\n",
		 order.product_id, order.quantity);

	 printf("Amount   : %.2f\n",
		 order.amount);

	 printf("Priority : %d%s\n", order.priority,
			 fraud ? " (fraud review)" : "");

	 if (log_event(&order, fraud ? "fraud-review" : "inventory-synced") == -1) {
			 stats_update(0, 0, 0, 1, 1, 0);
			 int recovery_result = log_order(&order);
			 if (recovery_result == -1) {
				 perror("log_order recovery");
				exit_status = EXIT_FAILURE;
			 }
		}

	 sem_post(empty_sem);

	 sleep(2);
    }

    close_queue(mq);
	stats_close();
	return exit_status;
}
