#include "../interprocess/spsc-queue-impl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <format>
#include <thread>

using namespace RingBuffer;
// using namespace RingBuffer::Interprocess;

template <typename T> class TestClassNotCopyable {
public:
  TestClassNotCopyable() { m_data = new T[1]; }

  explicit TestClassNotCopyable(std::size_t sz) { m_data = new T[sz]; }

  TestClassNotCopyable(const TestClassNotCopyable &) = delete;

  TestClassNotCopyable(TestClassNotCopyable &&rhs) noexcept {
    m_data = rhs.m_data;
    rhs.m_data = nullptr;
  }

  TestClassNotCopyable<T> &operator=(const TestClassNotCopyable &) = delete;

  TestClassNotCopyable<T> &operator=(TestClassNotCopyable &&rhs) noexcept {
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

TEST(InterprocessSpscQueue,
     SingleThreadBasicProduceThenConsumeWithoutInterface) {
  constexpr std::size_t sz = 63356;
  auto q1 = Interprocess::SpscQueue("test", true, sz);
  {
    auto q2 = Interprocess::SpscQueue("test", false, sz);

    std::string payload = "Hello world!";
    EXPECT_FALSE(q1.dequeue(payload));

    // assumption is that item_count will be smaller than 99999
    const auto item_count = sz / (payload.size() + 5 + sizeof(int)) - 1;
    for (std::size_t i = 0; i < item_count; ++i) {
      auto pl = payload + std::to_string(i);
      EXPECT_TRUE(q1.enqueue(payload + std::to_string(i)));
    }

    std::string payload2;
    for (std::size_t i = 0; i < item_count; ++i) {
      EXPECT_TRUE(q2.dequeue(payload2));
      EXPECT_EQ(payload + std::to_string(i), payload2);
    }
    EXPECT_FALSE(q2.dequeue(payload2));
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
    auto pl = payload + std::to_string(i);
    // TODO: currently q1.enqueue(payload + std::to_string(i)) does not work
    EXPECT_TRUE(q1.enqueue(pl));
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