#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "../interprocess/spsc-queue-impl.h"
#include "../intraprocess/spsc-queue-impl.h"
#include "../ringbuffer-interface.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <type_traits>

static volatile int ev_flag = 0;

template <class... T> constexpr bool always_false = false;

namespace RingBuffer {
struct Payload {
  uint32_t id;
  // int64_t unix_epoch_time_us;
  // char message[64 - sizeof(uint32_t) - sizeof(int64_t)];
};

inline void handle_signal(int) { ev_flag = 1; }

template <typename TImpl, typename T>
void producer_func(IRingBuffer<TImpl, T> &q) {
  using namespace std::chrono;
  uint32_t msg = 1;
  T raw_msg;

  while (!ev_flag) {
    if constexpr (std::is_same_v<T, uint32_t>) {
      raw_msg = msg;
    } else if constexpr (std::is_same_v<T, std::string>) {
      raw_msg.assign(reinterpret_cast<const char *>(&msg), sizeof(msg));
    } else {
      static_assert(always_false<T>, "Unsupported message type");
    }
    if (q.enqueue(raw_msg))
      msg++;
  }
}

template <typename TImpl, typename T>
void consumer_func(IRingBuffer<TImpl, T> &q) {
  using namespace std::chrono;
  auto t0 = duration_cast<milliseconds>(system_clock::now().time_since_epoch())
                .count();
  uint32_t t0_id = 0;
  uint32_t prev_msg = 0;
  while (!ev_flag) {
    T raw_msg;
    if (!q.dequeue(raw_msg))
      continue;
    std::cout << "dequeue()ed, head(): " << q.head() << ", tail(): " << q.tail()
              << "\n";
    uint32_t msg;
    if constexpr (std::is_same_v<T, uint32_t>) {
      msg = raw_msg;
    } else if constexpr (std::is_same_v<T, std::string>) {
      std::memcpy(&msg, raw_msg.data(), sizeof(msg));
    } else {
      static_assert(always_false<T>, "Unsupported message type");
    }
    if (prev_msg + 1 != msg) {
      std::cerr << "Unexpected message id: " << msg
                << ", prev_msg: " << prev_msg << ", raw_msg: " << raw_msg
                << std::endl;
    }
    prev_msg = msg;
    if (msg % 100'000'000 != 0)
      continue;
    const auto t1 =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count();
    if (t1 - t0 < 1000)
      continue;

    std::cout << "msg: " << msg << ", throughput: "
              << std::format("{:.1f}",
                             (msg - t0_id) / 1'000'000.0 / ((t1 - t0) / 1000.0))
              << "M msg/sec\n";
    t0 = t1;
    t0_id = msg;
  }
}

} // namespace RingBuffer

#endif // PAYLOAD_H
