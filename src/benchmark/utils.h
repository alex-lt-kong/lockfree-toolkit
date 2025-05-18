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
  uint64_t id;
  // int64_t unix_epoch_time_us;
  // char message[64 - sizeof(uint64_t) - sizeof(int64_t)];
};

inline void handle_signal(int) { ev_flag = 1; }

template <typename TImpl, typename T>
void producer_func(IRingBuffer<TImpl, T> &q) {
  using namespace std::chrono;
  uint64_t msg = 1;
  T raw_msg;
  while (!ev_flag) {
    if constexpr (std::is_same_v<T, uint64_t>) {
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
  uint64_t t0_id = 0;
  uint64_t prev_msg = 0;
  while (!ev_flag) {
    T raw_msg;
    if (!q.dequeue(raw_msg))
      continue;

    uint64_t msg;
    if constexpr (std::is_same_v<T, uint64_t>) {
      msg = raw_msg;
    } else if constexpr (std::is_same_v<T, std::string>) {
      std::memcpy(&msg, raw_msg.data(), sizeof(msg));
    } else {
      static_assert(always_false<T>, "Unsupported message type");
    }
    if (prev_msg + 1 != msg) {
      std::cerr << "Unexpected message id: " << msg
                << ", prev_msg: " << prev_msg << ", q.head():" << q.head()
                << ", q.tail(): " << q.tail() << std::endl;
      throw std::logic_error("Unexpected message id");
    }
    prev_msg = msg;
    if (msg % 10'000'000 != 0)
      continue;
    const auto t1 =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count();
    if (t1 - t0 < 5000)
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
