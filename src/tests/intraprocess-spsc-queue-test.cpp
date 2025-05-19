#include "../intraprocess/spsc-queue-beta-impl.h"
#include "../intraprocess/spsc-queue-impl.h"

#include <gtest/gtest.h>

#include <thread>

using namespace RingBuffer;
using namespace RingBuffer::Intraprocess;

template<typename T>
using SpscQueueImpl = SpscQueueBeta<T>;
//using SpscQueueImpl = SpscQueue<T>;

template<typename T>
class TestClassNotCopyable {
public:
  TestClassNotCopyable() { m_data = new T[1]; }

  explicit TestClassNotCopyable(std::size_t sz) { m_data = new T[sz]; }

  TestClassNotCopyable(const TestClassNotCopyable &) = delete;

  TestClassNotCopyable(TestClassNotCopyable &&rhs) noexcept {
    m_data = rhs.m_data;
    rhs.m_data = nullptr;
  }

  TestClassNotCopyable &operator=(const TestClassNotCopyable &) = delete;

  TestClassNotCopyable &operator=(TestClassNotCopyable &&rhs) noexcept {
    if (this != &rhs) {
      delete[] m_data;
      m_data = rhs.m_data;
      rhs.m_data = nullptr;
    }
    return *this;
  }

  ~TestClassNotCopyable() { delete[] m_data; }

  T get(int idx) const { return m_data[idx]; }

  void set(int idx, const T &val) const { m_data[idx] = val; }

  bool empty() const { return m_data == nullptr; }

private:
  T *m_data;
};

TEST(IntreprocessSpscQueue,
     SingleThreadBasicProduceThenConsumeWithoutInterface) {
  constexpr std::size_t sz = 63356;
  SpscQueueImpl<int> rb(sz);

  int ele;
  EXPECT_FALSE(rb.dequeue(ele));

  for (std::size_t i = 0; i < sz; i++) {
    EXPECT_TRUE(rb.enqueue(i));
  }
  for (std::size_t i = 0; i < sz; i++) {
    EXPECT_TRUE(rb.dequeue(ele));
    EXPECT_EQ(ele, i);
  }
  EXPECT_FALSE(rb.dequeue(ele));
}

template<typename Derived, typename T>
void func(IRingBuffer<Derived, T> &rb, const size_t sz) {
  int ele;
  EXPECT_FALSE(rb.dequeue(ele));

  for (std::size_t i = 0; i < sz; i++) {
    EXPECT_TRUE(rb.enqueue(i));
  }
  for (std::size_t i = 0; i < sz; i++) {
    EXPECT_TRUE(rb.dequeue(ele));
    EXPECT_EQ(ele, i);
  }
  EXPECT_FALSE(rb.dequeue(ele));
}

TEST(IntreprocessSpscQueue, SingleThreadBasicProduceThenConsumeWithInterface) {
  constexpr std::size_t sz = 63356;
  auto rb = SpscQueueImpl<int>(sz);
  func(rb, sz);
}

TEST(IntreprocessSpscQueue, SingleThreadCantCopyMoved) {
  constexpr std::size_t sz = 2;
  SpscQueueImpl<TestClassNotCopyable<std::string> > rb(sz);

  TestClassNotCopyable<std::string> ele1;
  // rb.dequeue(ele1);
  EXPECT_FALSE(rb.dequeue(ele1));

  TestClassNotCopyable<std::string> t{10};
  EXPECT_TRUE(rb.enqueue(std::move(t)));
  EXPECT_TRUE(t.empty());

  TestClassNotCopyable<std::string> ele2;
  EXPECT_TRUE(rb.dequeue(ele2));
  EXPECT_FALSE(rb.dequeue(ele2));
}

TEST(IntreprocessSpscQueue, FailedMoveShouldNotClearData) {
  constexpr std::size_t sz = 2;
  SpscQueueImpl<TestClassNotCopyable<std::string> > rb(sz);

  TestClassNotCopyable<std::string> ele;
  EXPECT_FALSE(rb.dequeue(ele));

  for (std::size_t i = 0; i < rb.capacity() + 1; i++) {
    constexpr std::size_t tsz = 10;
    TestClassNotCopyable<std::string> t{tsz};
    for (std::size_t j = 0; j < tsz; j++) {
      t.set(static_cast<int>(j), "test_string " + std::to_string(j));
    }
    if (i < rb.capacity())
      EXPECT_TRUE(rb.enqueue(std::move(t)));
    else {
      EXPECT_FALSE(rb.enqueue(std::move(t)));
      for (std::size_t j = 0; j < tsz; j++) {
        EXPECT_EQ(t.get(j), "test_string " + std::to_string(j));
      }
    }
  }
}

TEST(IntreprocessSpscQueue, SingleThreadProduceAndConsume) {
  constexpr std::size_t sz = 1;
  SpscQueueImpl<int> rb(sz);
  for (std::size_t i = 0; i < INT16_MAX; i++) {
    EXPECT_TRUE(rb.enqueue(i));
    int ele;
    EXPECT_TRUE(rb.dequeue(ele));
    EXPECT_EQ(ele, i);
  }
}

TEST(IntreprocessSpscQueue, SingleThreadCantCopyProduceAndConsume) {
  constexpr std::size_t sz = INT16_MAX;
  SpscQueueImpl<TestClassNotCopyable<std::string> > rb(sz);

  TestClassNotCopyable<std::string> ele;
  EXPECT_FALSE(rb.dequeue(ele));

  constexpr std::size_t tsz = 10;
  for (std::size_t i = 0; i < rb.capacity(); i++) {
    TestClassNotCopyable<std::string> t{tsz};
    for (int j = 0; j < tsz; j++) {
      t.set(j, std::to_string(i) + "/" + std::to_string(j));
    }
    EXPECT_TRUE(rb.enqueue(std::move(t)));
  }
  for (std::size_t i = 0; i < rb.capacity(); i++) {
    TestClassNotCopyable<std::string> ele;
    EXPECT_TRUE(rb.dequeue(ele));
    for (int j = 0; j < tsz; j++) {
      EXPECT_EQ(ele.get(j), std::to_string(i) + "/" + std::to_string(j));
    }
  }

  EXPECT_FALSE(rb.dequeue(ele));
}

TEST(IntreprocessSpscQueue, SingleThreadProduceOverflow) {
  constexpr std::size_t sz = INT8_MAX;
  SpscQueueImpl<int> rb(sz);
  for (std::size_t i = 0; i < INT16_MAX; i++) {
    if (i < sz)
      EXPECT_TRUE(rb.enqueue(i));
    else
      EXPECT_FALSE(rb.enqueue(i));
  }
}

TEST(IntreprocessSpscQueue, SingleThreadConsumeUnderflow) {
  constexpr std::size_t sz = INT8_MAX;
  SpscQueueImpl<int> rb(sz);
  for (std::size_t i = 0; i < sz * 2; i++) {
    if (i < sz)
      EXPECT_TRUE(rb.enqueue(i));
    else
      EXPECT_FALSE(rb.enqueue(i));
  }
  for (std::size_t i = 0; i < sz * 2; i++) {
    int ele;
    auto res = rb.dequeue(ele);
    if (i < sz) {
      EXPECT_TRUE(res);
      EXPECT_EQ(ele, i);
    } else {
      EXPECT_FALSE(res);
    }
  }
}

TEST(IntreprocessSpscQueue, SPSCCantCopy) {
  constexpr std::size_t sz = INT16_MAX / 4;
  constexpr std::size_t iter_size = INT16_MAX;
  SpscQueueImpl<TestClassNotCopyable<std::pair<int, int> > > rb(sz);
  constexpr std::size_t tsz = 10;
  auto producer = [&] {
    for (std::size_t i = 0; i < iter_size; ++i) {
      auto t = TestClassNotCopyable<std::pair<int, int> >(tsz);
      for (int j = 0; j < tsz; j++) {
        t.set(j, {i, j});
      }
      if (!rb.enqueue(std::move(t)))
        --i;
    }
  };
  auto consumer = [&] {
    std::size_t dequeue_count = 0;
    while (dequeue_count < iter_size) {
      TestClassNotCopyable<std::pair<int, int> > ele;
      if (rb.dequeue(ele)) {
        for (int j = 0; j < tsz; j++) {
          EXPECT_EQ(ele.get(j).first, dequeue_count);
          EXPECT_EQ(ele.get(j).second, j);
        }
        ++dequeue_count;
      }
    }
  };

  std::thread thread1(producer);
  std::thread thread2(consumer);

  thread1.join();
  thread2.join();

  TestClassNotCopyable<std::pair<int, int> > ele;
  EXPECT_FALSE(rb.dequeue(ele));
}

TEST(IntreprocessSpscQueue, SPSCDoesNotForceStdMove) {
  constexpr std::size_t sz = 10;
  SpscQueueImpl<int> rb(sz);
  for (int i = 0; i < sz; ++i) {
    EXPECT_TRUE(rb.enqueue(i));
  }
  for (int i = 0; i < sz; ++i) {
    int val;
    EXPECT_TRUE(rb.dequeue(val));
    EXPECT_EQ(val, i);
  }
}

TEST(IntreprocessSpscQueue, SPSCConcurrentProduceAndConsume) {
  // large iter_size takes time to complete, but it can expose rare race condition!
  constexpr std::uint64_t iter_size = 5'735'955'187;
  constexpr std::size_t qsz = 37;
  SpscQueueImpl<uint64_t> rb(qsz);
  auto producer = [&] {
    uint64_t enqueue_count = 0;
    while (enqueue_count < iter_size) {
      if (rb.enqueue(enqueue_count))
        ++enqueue_count;
    }
  };
  auto consumer = [&] {
    uint64_t dequeue_count = 0;
    while (dequeue_count < iter_size) {
      if (uint64_t ele; rb.dequeue(ele)) {
        EXPECT_EQ(ele, dequeue_count);
        ++dequeue_count;
      }
    }
  };

  std::thread thread1(producer);
  std::thread thread2(consumer);

  thread1.join();
  thread2.join();

  uint64_t ele;
  EXPECT_FALSE(rb.dequeue(ele));
}

TEST(IntreprocessSpscQueue, SPSCConcurrentProduceAndConsumeTightQueue) {
  std::vector<std::string> payloads = {
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
    std::string("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 11)
  };

  // large iter_size takes time to complete, but it can expose rare race condition!
  constexpr uint64_t iter_size = 5'935'955'521;
  constexpr std::size_t qsz = 2;
  SpscQueueImpl<std::string> rb(qsz);
  auto producer = [&] {
    uint64_t enqueue_count = 0;
    while (enqueue_count < iter_size) {
      if (rb.enqueue(payloads[enqueue_count % payloads.size()]))
        ++enqueue_count;
    }
  };
  auto consumer = [&] {
    uint64_t dequeue_count = 0;
    while (dequeue_count < iter_size) {
      if (std::string ele; rb.dequeue(ele)) {
        EXPECT_EQ(ele, payloads[dequeue_count % payloads.size()]);
        ++dequeue_count;
      }
    }
  };

  std::thread thread1(producer);
  std::thread thread2(consumer);

  thread1.join();
  thread2.join();

  std::string ele;
  EXPECT_FALSE(rb.dequeue(ele));
}
