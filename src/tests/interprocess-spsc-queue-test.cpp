#include "../interprocess/spsc-queue-impl.h"
#include "../interprocess/spsc-queue-beta-impl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <format>
#include <thread>

using namespace RingBuffer;

using SpscQueueImpl = Interprocess::SpscQueueBeta;
//using SpscQueueImpl = SpscQueue;

TEST(InterprocessSpscQueue,
     SingleThreadBasicProduceThenConsumeWithoutInterface) {
  constexpr std::size_t sz = 63356;
  auto q_con = SpscQueueImpl("test", true, sz);
  {
    auto q_prd = SpscQueueImpl("test", false, sz);

    std::string payload = "Hello world!";
    EXPECT_FALSE(q_con.dequeue(payload));

    // assumption is that item_count will be smaller than 99999
    const auto item_count = sz / (payload.size() + 5 + sizeof(int)) - 1;
    for (std::size_t i = 0; i < item_count; ++i) {
      auto pl = payload + std::to_string(i);
      EXPECT_TRUE(q_prd.enqueue(payload + std::to_string(i)));
    }

    std::string payload2;
    for (std::size_t i = 0; i < item_count; ++i) {
      EXPECT_TRUE(q_con.dequeue(payload2));
      EXPECT_EQ(payload + std::to_string(i), payload2);
    }
    EXPECT_FALSE(q_con.dequeue(payload2));
  }
}

template <typename Derived, typename T>
void func(IRingBuffer<Derived, T> &q1, const size_t sz) {
  auto q2 = SpscQueueImpl("test", false, sz);

  std::string payload = "Hello world!";
  EXPECT_FALSE(q1.dequeue(payload));

  // assumption is that item_count will be smaller than 99999
  const auto item_count = sz / (payload.size() + 5 + sizeof(int)) - 1;
  for (std::size_t i = 0; i < item_count; ++i) {
    // auto pl = payload + std::to_string(i);
    EXPECT_TRUE(q1.enqueue(payload + std::to_string(i)));
  }

  std::string payload2;
  for (std::size_t i = 0; i < item_count; ++i) {
    EXPECT_TRUE(q2.dequeue(payload2));
    EXPECT_EQ(payload + std::to_string(i), payload2);
  }
  EXPECT_FALSE(q2.dequeue(payload2));
}

TEST(InterprocessSpscQueue, SingleThreadBasicProduceThenConsumeWithInterface) {
  constexpr std::size_t sz = 63356;
  auto q1 = SpscQueueImpl("test", true, sz);
  func(q1, sz);
}

TEST(InterprocessSpscQueue, SingleThreadProduceAndConsume) {
  const std::size_t sz = (std::to_string(INT16_MAX).size() + 4) * 2;
  auto q1 = SpscQueueImpl("SingleThreadProduceAndConsume", true, sz);
  {
    auto q2 =
        SpscQueueImpl("SingleThreadProduceAndConsume", false, sz);
    for (std::size_t i = 0; i < INT16_MAX; i++) {
      EXPECT_TRUE(q1.enqueue(std::to_string(i)));
      std::string received;
      EXPECT_TRUE(q2.dequeue(received));
      EXPECT_EQ(received, std::to_string(i));
    }
  }
}

void common_consumer(const int qsz_bytes, const std::size_t iter_size,
                     const std::vector<std::string> &payloads,
                     const std::string &queue_name) {
  auto q = SpscQueueImpl(queue_name, true, qsz_bytes);
  std::size_t dequeue_count = 0;
  while (dequeue_count < iter_size) {
    // std::cout << "dequeue()ing..." << dequeue_count << std::endl;
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (std::string received; q.dequeue(received)) {
      EXPECT_EQ(received, payloads[dequeue_count % payloads.size()]);
      // std::cout << "dequeue()ed, dequeue_count: " << dequeue_count << "\n";
      ++dequeue_count;
    }
  }
}

void common_producer(const int qsz_bytes, const std::size_t iter_size,
                     const std::vector<std::string> &payloads,
                     const std::string &queue_name) {
  auto q = SpscQueueImpl(queue_name, false, qsz_bytes);
  std::size_t enqueue_count = 0;
  while (enqueue_count < iter_size) {
    // std::cout << "enqueue()ing..." << enqueue_count << std::endl;
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (q.enqueue(payloads[enqueue_count % payloads.size()])) {
      // std::cout << "enqueue()ed, enqueue_count: " << enqueue_count << "\n";
      ++enqueue_count;
    }
  }
}

TEST(InterprocessSpscQueue, ConcurrentProduceAndConsume) {
  std::vector<std::string> dummy_payloads = {
      "0xDeadBeef",
      "The quick brown fox jumps over the lazy dog",
      "Lorem ipsum dolor sit amet",
      "FooBar",
      "NullPointer",
      "Hello, World!",
      "42",
      "Spam and Eggs",
      "Hack the Planet",
      "TestString123",
      "Caffeinate and Dominate",
      "Underflow_Overflow",
      "NyanCat42",
      "TemporaryVariable",
      "SegFault",
      "QuantumFoam",
      "UndefinedBehavior",
      "CookieMonster",
      "404NotFound",
      "MagicNumber"};

  constexpr std::size_t qsz_bytes = 1024;
  constexpr std::size_t iter_size = INT32_MAX - 7537;

  std::thread thread_consumer(common_consumer, qsz_bytes, iter_size,
                              dummy_payloads, "ConcurrentProduceAndConsume");
  // Seems the setup of a shared memory queue is not atomic, need to wait to
  // avoid race condition
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::thread thread_producer(common_producer, qsz_bytes, iter_size,
                              dummy_payloads, "ConcurrentProduceAndConsume");
  thread_producer.join();
  thread_consumer.join();
}

TEST(InterprocessSpscQueue,
     ConcurrentProduceAndConsumeOnlyEmptyMessagesAndTightQueue) {
  std::vector<std::string> dummy_payloads = {""};
  constexpr std::size_t qsz_bytes = sizeof(int) * 2;
  constexpr std::size_t iter_size = INT32_MAX - 7919;

  std::thread thread_consumer(common_consumer, qsz_bytes, iter_size,
                              dummy_payloads,
                              "ConcurrentProduceAndConsumeOnlyEmptyMessages");
  // Seems the setup of a shared memory queue is not atomic, need to wait to
  // avoid race condition
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::thread thread_producer(common_producer, qsz_bytes, iter_size,
                              dummy_payloads,
                              "ConcurrentProduceAndConsumeOnlyEmptyMessages");

  thread_producer.join();
  thread_consumer.join();
}

TEST(InterprocessSpscQueue,
     ConcurrentProduceAndConsumeOnlyNullTerminatedCharMessagesAndTightQueue) {
  std::vector<std::string> dummy_payloads = {std::string("\x00", 1),
                                             std::string("\x00\x00", 2),
                                             std::string("\x00\x00\x00", 3)};
  constexpr std::size_t qsz_bytes = (sizeof(int) + 3) * 2;
  constexpr std::size_t iter_size = INT32_MAX - 7531;

  std::thread thread_consumer(
      common_consumer, qsz_bytes, iter_size, dummy_payloads,
      "ConcurrentProduceAndConsumeOnlyNullTerminatedCharMessagesAndTightQueue");
  // Seems the setup of a shared memory queue is not atomic, need to wait to
  // avoid race condition
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::thread thread_producer(
      common_producer, qsz_bytes, iter_size, dummy_payloads,
      "ConcurrentProduceAndConsumeOnlyNullTerminatedCharMessagesAndTightQueue");

  thread_producer.join();
  thread_consumer.join();
}

TEST(InterprocessSpscQueue, ConcurrentProduceAndConsumeMixedEdgeCases) {
  std::vector<std::string> dummy_payloads = {
      "0xDeadBeef",
      std::string("\x00\x12\x34\x56\x78\x9A\xBC\xDE", 8),
      "The quick brown fox jumps over the lazy dog",
      "Lorem ipsum dolor sit amet",
      "FooBar",
      "",
      "",
      std::string("\x00", 1),
      "",
      "NullPointer",
      "Hello, World!",
      "42",
      "",
      std::string("\x00", 1),
      std::string("\x00\x00", 2),
      std::string("\x00\x00\x00", 3),
      std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 11)};

  const std::size_t qsz_bytes = (sizeof(int) + dummy_payloads[2].size()) * 2;
  constexpr std::size_t iter_size = INT32_MAX - 157;

  std::thread thread_consumer(common_consumer, qsz_bytes, iter_size,
                              dummy_payloads,
                              "ConcurrentProduceAndConsumeEdgeCases");
  // Seems the setup of a shared memory queue is not atomic, need to wait to
  // avoid race condition
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::thread thread_producer(common_producer, qsz_bytes, iter_size,
                              dummy_payloads,
                              "ConcurrentProduceAndConsumeEdgeCases");
  thread_producer.join();
  thread_consumer.join();
}
