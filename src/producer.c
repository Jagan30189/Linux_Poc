#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>

#include "order.h"
#include "mq_manager.h"
#include "semaphore_manager.h"
#include "stats_manager.h"

static int read_text(const char *prompt, char *value, size_t value_size)
{
	printf("%s", prompt);
	fflush(stdout);
	if (fgets(value, value_size, stdin) == NULL)
		return -1;

	value[strcspn(value, "\n")] = '\0';
	return value[0] == '\0' ? -1 : 0;
}

static int read_int(const char *prompt, int *value)
{
	char input[64];
	char *end;
	long parsed;

	if (read_text(prompt, input, sizeof(input)) == -1)
		return -1;
	errno = 0;
	parsed = strtol(input, &end, 10);
	while (isspace((unsigned char)*end)) end++;
	if (errno != 0 || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX)
		return -1;
	*value = (int)parsed;
	return 0;
}

static int read_amount(const char *prompt, double *value)
{
	char input[64];
	char *end;

	if (read_text(prompt, input, sizeof(input)) == -1)
		return -1;
	errno = 0;
	*value = strtod(input, &end);
	while (isspace((unsigned char)*end)) end++;
	return errno == 0 && *end == '\0' && *value > 0.0 ? 0 : -1;
}

static int read_manual_order(Order *order)
{
	printf("\nE-Commerce Order Entry\n");
	if (read_int("Enter Order ID: ", &order->order_id) == -1 ||
		read_text("Enter Customer Name: ", order->customer_name,
			   sizeof(order->customer_name)) == -1 ||
		read_int("Enter Product ID: ", &order->product_id) == -1 ||
		read_int("Enter Quantity: ", &order->quantity) == -1 ||
		read_amount("Enter Amount: ", &order->amount) == -1 ||
		read_int("Enter Priority (1-3): ", &order->priority) == -1 ||
		order->order_id < 1 || order->product_id < 1 || order->quantity < 1 ||
		order->priority < 1 || order->priority > 3) {
		fprintf(stderr, "Invalid order input.\n");
		return -1;
	}
	return 0;
}

static int select_mode(int *manual_mode, int *producer_id, int *order_count)
{
	int choice;

	printf("================================\n");
	printf(" E-COMMERCE ORDER PROCESSING\n");
	printf("================================\n");
	printf("1. Manual Order Entry\n");
	printf("2. Automatic Order Generation\n");
	printf("3. Exit\n\n");
	if (read_int("Enter your choice: ", &choice) == -1 || choice < 1 || choice > 3)
		return -1;
	if (choice == 3)
		return 1;

	*manual_mode = choice == 1;
	*producer_id = 1;
	if (read_int("Enter number of orders: ", order_count) == -1)
		return -1;
	return 0;
}

int main(int argc, char **argv)
{
	mqd_t mq;
	int producer_id = 1;
	int order_count = 10;
	int manual_mode = 0;
	int exit_status = EXIT_SUCCESS;

	if (argc == 1) {
		int mode_result = select_mode(&manual_mode, &producer_id, &order_count);
		if (mode_result == 1)
			return EXIT_SUCCESS;
		if (mode_result == -1)
			return EXIT_FAILURE;
	} else if (argc == 2 && strcmp(argv[1], "manual") == 0) {
		manual_mode = 1;
		if (read_int("Enter number of orders: ", &order_count) == -1)
			return EXIT_FAILURE;
	} else if (argc == 4 && strcmp(argv[1], "manual") == 0) {
		manual_mode = 1;
		producer_id = atoi(argv[2]);
		order_count = atoi(argv[3]);
	} else if (argc == 3) {
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
	if (stats_open(producer_id == 1) == -1) {
		perror("stats_open");
		close_queue(mq);
		return EXIT_FAILURE;
	}

	for (int i = 1; i <= order_count; i++)
	{
		Order order = {0};

		if (manual_mode) {
			if (read_manual_order(&order) == -1) {
				exit_status = EXIT_FAILURE;
				break;
			}
		} else {
			order.order_id = producer_id * 1000 + i;
			snprintf(order.customer_name, sizeof(order.customer_name),
					 "Customer_%d", order.order_id);
			order.product_id = 500 + i;
			order.quantity = (i % 3) + 1;
			order.amount = i * 100.0;
			order.priority = (i % 3) + 1;
		}

		if (sem_wait(empty_sem) == -1) {
			perror("sem_wait");
			exit_status = EXIT_FAILURE;
			break;
		}

		if (send_order(mq, &order) == -1) {
			perror("mq_send");
			sem_post(empty_sem);
			exit_status = EXIT_FAILURE;
			break;
		}
		stats_update(1, 0, 0, 0, 0, 0);

		printf("Produced Order %d\n",
			   order.order_id);
		if (manual_mode)
			printf("Order generated successfully! Sending order to message queue...\n");

		sem_post(full_sem);

		sleep(1);
	}

	close_queue(mq);
	stats_close();
	return exit_status;
}
