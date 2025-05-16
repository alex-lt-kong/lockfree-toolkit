#include "../intraprocess/spsc-queue-impl.h"
#include "../ringbuffer-interface.h"
#include "payload.h"

#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

using namespace RingBuffer;

static volatile int ev_flag = 0;

constexpr size_t q_size = INT16_MAX;
void handle_signal(int) { ev_flag = 1; }

template <typename TImpl, typename T>
void __attribute__((noinline)) producer_func(IRingBuffer<TImpl, T> &q) {
  using namespace std::chrono;
  uint64_t msg = 1;
  while (!ev_flag) {
    if (q.enqueue(msg))
      msg++;
  }
}

template <typename TImpl, typename T>
void __attribute__((noinline)) consumer_func(IRingBuffer<TImpl, T> &q) {
  using namespace std::chrono;
  auto t0 = duration_cast<milliseconds>(system_clock::now().time_since_epoch())
                .count();
  uint64_t prev_msg = 0;
  uint64_t msg;
  while (!ev_flag) {
    if (!q.dequeue(msg))
      continue;
    if (prev_msg + 1 != msg) {
      std::cerr << "Unexpected message id: " << msg << std::endl;
    }
    prev_msg = msg;
    if (msg % 10'000'001 != 0)
      continue;
    const auto t1 =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count();
    if (t1 - t0 < 1000)
      continue;
    std::cout << msg
              << ", throughput: " << (msg / 1'000'000) / ((t1 - t0) / 1000)
              << "Mpps\n";
  }
}

int main() {
  if (signal(SIGINT, handle_signal) == SIG_ERR) {
    perror("signal()");
    return EXIT_FAILURE;
  }

  Intraprocess::SpscQueue<uint64_t> q{1'000'000};
  // consumer_func<Intraprocess::SpscQueue<Payload>, Payload>(q);
  std::thread thread_consumer(
      consumer_func<Intraprocess::SpscQueue<uint64_t>, uint64_t>, std ::ref(q));
  std::thread thread_producer(
      producer_func<Intraprocess::SpscQueue<uint64_t>, uint64_t>, std::ref(q));

  thread_consumer.join();
  thread_producer.join();

  return 0;
}