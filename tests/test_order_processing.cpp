#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "logger.h"
#include "mq_manager.h"
#include "semaphore_manager.h"

int consumer_main(int argc, char **argv);
int producer_main(int argc, char **argv);
}

namespace {

class OrderProcessingTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		mkdir("logs", 0777);
		std::remove("logs/orders.log");
		unlink_queue();
		cleanup_semaphores();
		ASSERT_EQ(init_semaphores(), 0);
	}

	void TearDown() override
	{
		cleanup_semaphores();
		unlink_queue();
		std::remove("logs/orders.log");
	}
};

TEST_F(OrderProcessingTest, SendsAndReceivesOrderWithPriority)
{
	mqd_t queue = create_queue();
	ASSERT_NE(queue, (mqd_t)-1);

	Order sent = {};
	sent.order_id = 42;
	std::snprintf(sent.customer_name, sizeof(sent.customer_name), "Customer_42");
	sent.amount = 123.45;
	sent.priority = 3;
	Order received = {};
	ASSERT_EQ(send_order(queue, &sent), 0);
	ASSERT_EQ(receive_order(queue, &received), 0);

	EXPECT_EQ(received.order_id, sent.order_id);
	EXPECT_STREQ(received.customer_name, sent.customer_name);
	EXPECT_DOUBLE_EQ(received.amount, sent.amount);
	EXPECT_EQ(received.priority, sent.priority);

	close_queue(queue);
	unlink_queue();
}

TEST_F(OrderProcessingTest, ReceivesHigherPriorityOrderFirst)
{
	mqd_t queue = create_queue();
	ASSERT_NE(queue, (mqd_t)-1);

	Order low_priority = {};
	low_priority.order_id = 1;
	low_priority.priority = 1;
	Order high_priority = {};
	high_priority.order_id = 2;
	high_priority.priority = 3;
	Order received = {};

	ASSERT_EQ(send_order(queue, &low_priority), 0);
	ASSERT_EQ(send_order(queue, &high_priority), 0);
	ASSERT_EQ(receive_order(queue, &received), 0);
	EXPECT_EQ(received.order_id, high_priority.order_id);
	EXPECT_EQ(received.priority, high_priority.priority);

	close_queue(queue);
	unlink_queue();
}

TEST_F(OrderProcessingTest, FailsToOpenMissingQueue)
{
	unlink_queue();
	EXPECT_EQ(open_queue(), (mqd_t)-1);
}

TEST_F(OrderProcessingTest, OpensExistingQueue)
{
	mqd_t created_queue = create_queue();
	ASSERT_NE(created_queue, (mqd_t)-1);
	close_queue(created_queue);

	mqd_t opened_queue = open_queue();
	ASSERT_NE(opened_queue, (mqd_t)-1);
	close_queue(opened_queue);
	unlink_queue();
}

TEST_F(OrderProcessingTest, OpensExistingSemaphores)
{
	ASSERT_EQ(init_semaphores(), 0);
	ASSERT_NE(empty_sem, SEM_FAILED);
	ASSERT_NE(full_sem, SEM_FAILED);
	ASSERT_NE(log_sem, SEM_FAILED);

	sem_close(empty_sem);
	sem_close(full_sem);
	sem_close(log_sem);
	empty_sem = nullptr;
	full_sem = nullptr;
	log_sem = nullptr;

	ASSERT_EQ(open_semaphores(), 0);
	EXPECT_NE(empty_sem, SEM_FAILED);
	EXPECT_NE(full_sem, SEM_FAILED);
	EXPECT_NE(log_sem, SEM_FAILED);
}

TEST_F(OrderProcessingTest, FailsToOpenMissingSemaphores)
{
	cleanup_semaphores();
	EXPECT_EQ(open_semaphores(), -1);
	empty_sem = nullptr;
	full_sem = nullptr;
	log_sem = nullptr;
}

TEST_F(OrderProcessingTest, LogsOrder)
{
	Order order = {};
	order.order_id = 7;
	std::snprintf(order.customer_name, sizeof(order.customer_name), "Customer_7");
	order.amount = 75.0;
	order.priority = 2;
	ASSERT_EQ(log_order(&order), 0);

	std::ifstream log("logs/orders.log");
	ASSERT_TRUE(log.is_open());
	std::string line;
	ASSERT_TRUE(std::getline(log, line));
	EXPECT_EQ(line, "OrderID=7 Customer=Customer_7 Amount=75.00 Priority=2");
}

TEST_F(OrderProcessingTest, FailsToLogWhenLogDirectoryIsMissing)
{
	ASSERT_EQ(rmdir("logs"), 0);

	Order order = {};
	order.order_id = 8;
	EXPECT_EQ(log_order(&order), -1);
	ASSERT_EQ(mkdir("logs", 0777), 0);
}

TEST(ApplicationEntryPoints, RejectInvalidConsumerArguments)
{
	char program[] = "consumer";
	char invalid_argument[] = "1";
	char *argv[] = {program, invalid_argument, invalid_argument, invalid_argument, nullptr};

	EXPECT_EQ(consumer_main(4, argv), EXIT_FAILURE);
}

TEST(ApplicationEntryPoints, RejectInvalidProducerArguments)
{
	char program[] = "producer";
	char invalid_argument[] = "1";
	char *argv[] = {program, invalid_argument, invalid_argument, invalid_argument, nullptr};

	EXPECT_EQ(producer_main(4, argv), EXIT_FAILURE);
}

TEST(ApplicationEntryPoints, RejectNonPositiveConsumerCount)
{
	char program[] = "consumer";
	char consumer_id[] = "1";
	char order_count[] = "0";
	char *argv[] = {program, consumer_id, order_count, nullptr};

	EXPECT_EQ(consumer_main(3, argv), EXIT_FAILURE);
}

TEST(ApplicationEntryPoints, RejectNonPositiveProducerId)
{
	char program[] = "producer";
	char producer_id[] = "0";
	char order_count[] = "1";
	char *argv[] = {program, producer_id, order_count, nullptr};

	EXPECT_EQ(producer_main(3, argv), EXIT_FAILURE);
}

TEST(ApplicationEntryPoints, ProcessesSingleOrder)
{
	char producer_program[] = "producer";
	char producer_id[] = "1";
	char producer_count[] = "1";
	char *producer_argv[] = {
		producer_program, producer_id, producer_count, nullptr
	};
	ASSERT_EQ(producer_main(3, producer_argv), EXIT_SUCCESS);

	char consumer_program[] = "consumer";
	char consumer_count[] = "1";
	char *consumer_argv[] = {consumer_program, consumer_count, nullptr};
	EXPECT_EQ(consumer_main(2, consumer_argv), EXIT_SUCCESS);
}

}  // namespace
