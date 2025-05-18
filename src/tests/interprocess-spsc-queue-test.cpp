#include "../interprocess/spsc-queue-impl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <format>
#include <thread>

using namespace RingBuffer;
// using namespace RingBuffer::Interprocess;

TEST(InterprocessSpscQueue,
     SingleThreadBasicProduceThenConsumeWithoutInterface) {
  constexpr std::size_t sz = 63356;
  auto q_con = Interprocess::SpscQueue("test", true, sz);
  {
    auto q_prd = Interprocess::SpscQueue("test", false, sz);

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
  auto q2 = Interprocess::SpscQueue("test", false, sz);

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
  auto q1 = Interprocess::SpscQueue("test", true, sz);
  func(q1, sz);
}

TEST(InterprocessSpscQueue, SingleThreadProduceAndConsume) {
  const std::size_t sz = (std::to_string(INT16_MAX).size() + 4) * 2;
  auto q1 = Interprocess::SpscQueue("SingleThreadProduceAndConsume", true, sz);
  {
    auto q2 =
        Interprocess::SpscQueue("SingleThreadProduceAndConsume", false, sz);
    for (std::size_t i = 0; i < INT16_MAX; i++) {
      EXPECT_TRUE(q1.enqueue(std::to_string(i)));
      std::string received;
      EXPECT_TRUE(q2.dequeue(received));
      EXPECT_EQ(received, std::to_string(i));
    }
  }
}

TEST(InterprocessSpscQueue, SingleThreadProduceOverflow) {
  constexpr auto ele_count_approx = INT16_MAX;
  const std::size_t qsz_bytes =
      (std::to_string(INT16_MAX).size() + sizeof(int)) * ele_count_approx;
  auto q =
      Interprocess::SpscQueue("SingleThreadProduceOverflow", true, qsz_bytes);
  size_t used_space_bytes = 0;
  for (size_t i = 0; i < (static_cast<uint32_t>(INT32_MAX) - 1) * 2; i++) {
    std::string pl = std::to_string(i);
    used_space_bytes += sizeof(int) + pl.size();
    // EXPECT_EQ(q.get_used_bytes(), used_space_bytes);
    //  supposed to be implementation details, but let's test it anyways
    EXPECT_EQ(q.enqueue(pl),
              used_space_bytes <
                  qsz_bytes - (std::to_string(INT16_MAX).size() + sizeof(int)))
        << "used_space_bytes: " << used_space_bytes << ", sz: " << qsz_bytes;
  }
}

TEST(InterprocessSpscQueue, SingleThreadConsumeUnderflow) {
  constexpr auto ele_count_approx = INT16_MAX;
  const std::size_t qsz_bytes =
      (std::to_string(INT16_MAX).size() + sizeof(int)) * ele_count_approx;
  auto q1 =
      Interprocess::SpscQueue("SingleThreadConsumeUnderflow", true, qsz_bytes);
  size_t q1_used_space_bytes = 0;
  for (size_t i = 0; i < (static_cast<uint32_t>(INT32_MAX) - 1) * 2; i++) {
    std::string pl = std::to_string(i);
    q1_used_space_bytes += sizeof(int) + pl.size();
    // supposed to be implementation details, but let's test it anyways
    EXPECT_EQ(q1.enqueue(pl),
              q1_used_space_bytes <
                  qsz_bytes - (std::to_string(INT16_MAX).size() + sizeof(int)))
        << "used_space_bytes: " << q1_used_space_bytes
        << ", qsz_bytes: " << qsz_bytes;
  }
  auto q2 =
      Interprocess::SpscQueue("SingleThreadConsumeUnderflow", false, qsz_bytes);
  size_t q2_used_space_bytes = 0;
  for (size_t i = 0; i < (static_cast<uint32_t>(INT32_MAX) - 1) * 2; i++) {
    std::string pl = std::to_string(i);
    q2_used_space_bytes += sizeof(int) + pl.size();
    // supposed to be implementation details, but let's test it anyways
    auto res = q2.dequeue(pl);
    EXPECT_EQ(res,
              q2_used_space_bytes <
                  qsz_bytes - (std::to_string(INT16_MAX).size() + sizeof(int)))
        << "q2_used_space_bytes: " << q2_used_space_bytes
        << ", qsz_bytes: " << qsz_bytes;
    if (res) {
      EXPECT_EQ(pl, std::to_string(i));
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

  auto producer = [&] {
    auto q = Interprocess::SpscQueue("ConcurrentProduceAndConsume", false,
                                     qsz_bytes);
    std::size_t enqueue_count = 0;
    while (enqueue_count < iter_size) {
      if (q.enqueue(dummy_payloads[enqueue_count % dummy_payloads.size()])) {
        ++enqueue_count;
      }
    }
  };
  auto consumer = [&] {
    auto q =
        Interprocess::SpscQueue("ConcurrentProduceAndConsume", true, qsz_bytes);
    std::size_t dequeue_count = 0;
    while (dequeue_count < iter_size) {
      if (std::string received; q.dequeue(received)) {
        EXPECT_EQ(received,
                  dummy_payloads[dequeue_count % dummy_payloads.size()]);
        ++dequeue_count;
      }
    }
  };

  std::thread thread_consumer(consumer);
  // Seems the setup of a shared memory queue is not atomic, need to wait to
  // avoid race condition
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  std::thread thread_producer(producer);
  thread_producer.join();
  thread_consumer.join();
}

TEST(InterprocessSpscQueue, ConcurrentProduceAndConsumeEdgeCases) {
  std::vector<std::string> dummy_payloads = {
      "0xDeadBeef",
      std::string("\x00\x12\x34\x56\x78\x9A\xBC\xDE", 8),
      "The quick brown fox jumps over the lazy dog",
      "Lorem ipsum dolor sit amet",
      "FooBar",
      "",
      "NullPointer",
      "Hello, World!",
      "42",
      "",
      std::string("\x00", 1),
      std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 11)};

  constexpr std::size_t qsz_bytes = 1151;
  constexpr std::size_t iter_size = INT32_MAX - 157;

  auto producer = [&] {
    auto q = Interprocess::SpscQueue("ConcurrentProduceAndConsumeEdgeCases",
                                     false, qsz_bytes);
    for (int64_t i = 0; i < iter_size; ++i) {
      if (!q.enqueue(dummy_payloads[i % dummy_payloads.size()])) {
        --i;
      }
    }
  };
  auto consumer = [&] {
    auto q = Interprocess::SpscQueue("ConcurrentProduceAndConsumeEdgeCases",
                                     true, qsz_bytes);
    std::size_t dequeue_count = 0;
    while (dequeue_count < iter_size) {
      if (std::string received; q.dequeue(received)) {
        EXPECT_EQ(received,
                  dummy_payloads[dequeue_count % dummy_payloads.size()]);
        ++dequeue_count;
      }
    }
  };

  std::thread thread_consumer(consumer);
  // Seems the setup of a shared memory queue is not atomic, need to wait to
  // avoid race condition
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  std::thread thread_producer(producer);
  thread_producer.join();
  thread_consumer.join();
}
