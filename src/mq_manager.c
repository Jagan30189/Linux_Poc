#include <fcntl.h>
#include <errno.h>
#include "mq_manager.h"

mqd_t create_queue()
{
	struct mq_attr attr;

	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_MSG;
	attr.mq_msgsize = sizeof(Order);
	attr.mq_curmsgs = 0;

	return mq_open(MQ_NAME,
				   O_CREAT | O_RDWR,
				   0666,
				   &attr);
}

mqd_t open_queue()
{
	return mq_open(MQ_NAME, O_RDWR);
}

int send_order(mqd_t mq, const Order *order)
{
	return mq_send(mq,
				   (const char *)order,
				   sizeof(Order),
				   (unsigned int)order->priority);
}

int receive_order(mqd_t mq, Order *order)
{
	unsigned int priority;

	return mq_receive(mq,
					  (char *)order,
					  sizeof(Order),
					  &priority) == -1 ? -1 : 0;
}

void close_queue(mqd_t mq)
{
	mq_close(mq);
}

void unlink_queue()
{
	mq_unlink(MQ_NAME);
}
