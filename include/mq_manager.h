#ifndef MQ_MANAGER_H
#define MQ_MANAGER_H

#include <mqueue.h>
#include "order.h"

#define MQ_NAME "/order_queue"
#define MAX_MSG 5

mqd_t create_queue();
mqd_t open_queue();

int send_order(mqd_t mq, const Order *order);

int receive_order(mqd_t mq, Order *order);

void close_queue(mqd_t mq);

void unlink_queue();

#endif
