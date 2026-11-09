CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -Iinclude
LDLIBS = -lrt -pthread
COMMON = src/mq_manager.c src/semaphore_manager.c src/logger.c src/stats_manager.c

.PHONY: all producer consumer order_system monitor clean clean-ipc run run-example gtest valgrind cppcheck misra coverage

all: producer consumer order_system monitor

producer: src/producer.c $(COMMON) include/order.h include/mq_manager.h include/semaphore_manager.h
	$(CC) $(CFLAGS) -o $@ src/producer.c $(COMMON) $(LDLIBS)

consumer: src/consumer.c $(COMMON) include/order.h include/mq_manager.h include/semaphore_manager.h include/logger.h
	$(CC) $(CFLAGS) -o $@ src/consumer.c $(COMMON) $(LDLIBS)

order_system: src/order_system.c src/mq_manager.c src/semaphore_manager.c src/stats_manager.c include/order.h include/mq_manager.h include/semaphore_manager.h include/stats_manager.h
	$(CC) $(CFLAGS) -o $@ src/order_system.c src/mq_manager.c src/semaphore_manager.c src/stats_manager.c $(LDLIBS)

monitor: src/monitor.c src/mq_manager.c src/stats_manager.c include/order.h include/mq_manager.h include/stats_manager.h
	$(CC) $(CFLAGS) -o $@ src/monitor.c src/mq_manager.c src/stats_manager.c $(LDLIBS)

clean:
	rm -f producer consumer order_system monitor $(TEST_BINARY) $(TEST_OBJECTS) $(TEST_ENTRY_OBJECTS) $(COVERAGE_BINARY) $(COVERAGE_OBJECTS) $(COVERAGE_ENTRY_OBJECTS) $(COVERAGE_PROGRAMS)
	rm -f *.gcov
	rm -rf coverage
	rm -f logs/orders.log

clean-ipc:
	-rm -f /dev/mqueue/order_queue
	-rm -f /dev/shm/sem.empty_sem /dev/shm/sem.full_sem /dev/shm/sem.log_sem /dev/shm/sem.stats_mutex
	-rm -f /dev/shm/ecommerce_order_stats

run-example: all
	$(MAKE) clean-ipc
	mkdir -p logs
	: > logs/orders.log
	./order_system

run: run-example

TEST_CXX = g++
TEST_CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -Iinclude
TEST_LDLIBS = -lgtest -lgtest_main -pthread -lrt
TEST_BINARY = tests/order_processing_test
TEST_OBJECTS = tests/mq_manager.o tests/semaphore_manager.o tests/logger.o tests/stats_manager.o
TEST_ENTRY_OBJECTS = tests/consumer_main.o tests/producer_main.o

$(TEST_BINARY): tests/test_order_processing.cpp $(TEST_OBJECTS) $(TEST_ENTRY_OBJECTS) include/order.h include/mq_manager.h include/semaphore_manager.h include/logger.h
	@command -v $(TEST_CXX) >/dev/null || { echo "g++ is required for gtest tests."; exit 1; }
	@$(TEST_CXX) -x c++ -E -include gtest/gtest.h /dev/null >/dev/null 2>&1 || { echo "GoogleTest headers are required (install libgtest-dev)."; exit 1; }
	$(TEST_CXX) $(TEST_CXXFLAGS) -o $@ tests/test_order_processing.cpp $(TEST_OBJECTS) $(TEST_ENTRY_OBJECTS) $(TEST_LDLIBS)

tests/%.o: src/%.c
	mkdir -p tests
	$(CC) $(CFLAGS) -c -o $@ $<

tests/consumer_main.o: src/consumer.c
	mkdir -p tests
	$(CC) $(CFLAGS) -Dmain=consumer_main -c -o $@ $<

tests/producer_main.o: src/producer.c
	mkdir -p tests
	$(CC) $(CFLAGS) -Dmain=producer_main -c -o $@ $<

gtest: $(TEST_BINARY)
	mkdir -p logs
	./$(TEST_BINARY)

valgrind: $(TEST_BINARY)
	mkdir -p logs
	valgrind --leak-check=full --error-exitcode=1 ./$(TEST_BINARY)

cppcheck:
	@command -v cppcheck >/dev/null || { echo "cppcheck is required."; exit 1; }
	cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem -Iinclude src

misra:
	@command -v cppcheck >/dev/null || { echo "cppcheck is required for MISRA analysis."; exit 1; }
	cppcheck --addon=misra --enable=style --error-exitcode=1 -Iinclude src

COVERAGE_CFLAGS = $(CFLAGS) --coverage
COVERAGE_CXXFLAGS = $(TEST_CXXFLAGS) --coverage
COVERAGE_BINARY = coverage/order_processing_test
COVERAGE_OBJECTS = coverage/mq_manager.o coverage/semaphore_manager.o coverage/logger.o coverage/stats_manager.o
COVERAGE_ENTRY_OBJECTS = coverage/consumer_main.o coverage/producer_main.o
COVERAGE_PROGRAMS = coverage/producer coverage/consumer

coverage/%.o: src/%.c
	mkdir -p coverage
	$(CC) $(COVERAGE_CFLAGS) -c -o $@ $<

$(COVERAGE_BINARY): tests/test_order_processing.cpp $(COVERAGE_OBJECTS) $(COVERAGE_ENTRY_OBJECTS) include/order.h include/mq_manager.h include/semaphore_manager.h include/logger.h
	@command -v $(TEST_CXX) >/dev/null || { echo "g++ is required for coverage tests."; exit 1; }
	@$(TEST_CXX) -x c++ -E -include gtest/gtest.h /dev/null >/dev/null 2>&1 || { echo "GoogleTest headers are required (install libgtest-dev)."; exit 1; }
	mkdir -p coverage
	$(TEST_CXX) $(COVERAGE_CXXFLAGS) -o $@ tests/test_order_processing.cpp $(COVERAGE_OBJECTS) $(COVERAGE_ENTRY_OBJECTS) $(TEST_LDLIBS)

coverage/consumer_main.o: src/consumer.c
	mkdir -p coverage
	$(CC) $(COVERAGE_CFLAGS) -Dmain=consumer_main -c -o $@ $<

coverage/producer_main.o: src/producer.c
	mkdir -p coverage
	$(CC) $(COVERAGE_CFLAGS) -Dmain=producer_main -c -o $@ $<

coverage/producer: src/producer.c $(COVERAGE_OBJECTS) include/order.h include/mq_manager.h include/semaphore_manager.h
	mkdir -p coverage
	$(CC) $(COVERAGE_CFLAGS) -o $@ src/producer.c $(COVERAGE_OBJECTS) $(LDLIBS) --coverage

coverage/consumer: src/consumer.c $(COVERAGE_OBJECTS) include/order.h include/mq_manager.h include/semaphore_manager.h include/logger.h
	mkdir -p coverage
	$(CC) $(COVERAGE_CFLAGS) -o $@ src/consumer.c $(COVERAGE_OBJECTS) $(LDLIBS) --coverage

coverage: $(COVERAGE_BINARY) $(COVERAGE_PROGRAMS)
	mkdir -p logs
	./$(COVERAGE_BINARY)
	$(MAKE) clean-ipc
	: > logs/orders.log
	./coverage/producer 1 1 & producer_pid=$$!; \
	sleep 1; \
	./coverage/consumer 1 1; \
	wait $$producer_pid
	mkdir -p coverage/report
	gcov -b -c -r -o coverage/producer-producer.gcno src/producer.c
	gcov -b -c -r -o coverage/consumer-consumer.gcno src/consumer.c
	gcov -b -c -r -o coverage/mq_manager.gcno src/mq_manager.c
	gcov -b -c -r -o coverage/semaphore_manager.gcno src/semaphore_manager.c
	gcov -b -c -r -o coverage/logger.gcno src/logger.c
	mv -f *.gcov coverage/report/
