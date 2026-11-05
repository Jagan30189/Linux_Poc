CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -Iinclude
LDLIBS = -lrt -pthread
COMMON = src/mq_manager.c src/semaphore_manager.c src/logger.c

.PHONY: all clean clean-ipc run-example

all: producer consumer

producer: src/producer.c $(COMMON) include/order.h include/mq_manager.h include/semaphore_manager.h
	$(CC) $(CFLAGS) -o $@ src/producer.c $(COMMON) $(LDLIBS)

consumer: src/consumer.c $(COMMON) include/order.h include/mq_manager.h include/semaphore_manager.h include/logger.h
	$(CC) $(CFLAGS) -o $@ src/consumer.c $(COMMON) $(LDLIBS)

clean:
	rm -f producer consumer
	rm -f logs/orders.log

clean-ipc:
	-rm -f /dev/mqueue/order_queue
	-rm -f /dev/shm/sem.empty_sem /dev/shm/sem.full_sem /dev/shm/sem.log_sem

run-example: all
	$(MAKE) clean-ipc
	mkdir -p logs
	: > logs/orders.log
	./producer 1 5 & producer_one=$$!; \
	./producer 2 5 & producer_two=$$!; \
	./consumer 1 5 & consumer_one=$$!; \
	./consumer 2 5 & consumer_two=$$!; \
	wait $$producer_one $$producer_two $$consumer_one $$consumer_two
