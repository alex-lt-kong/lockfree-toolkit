#include "utils.h"

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

using namespace RingBuffer;

//template <typename T> using SpscQueueImpl = Intraprocess::SpscQueueBeta<T>;
template <typename T>  using SpscQueueImpl = Intraprocess::SpscQueue<T>;

constexpr size_t q_size = INT16_MAX;

int main() {
  if (signal(SIGINT, handle_signal) == SIG_ERR ||
      signal(SIGTERM, handle_signal) == SIG_ERR) {
    perror("signal()");
    return EXIT_FAILURE;
  }

  SpscQueueImpl<uint64_t> q{1'000'000};
  std::thread thread_consumer(consumer_func<SpscQueueImpl<uint64_t>, uint64_t>,
                              std ::ref(q));
  std::thread thread_producer(producer_func<SpscQueueImpl<uint64_t>, uint64_t>,
                              std::ref(q));

  thread_consumer.join();
  thread_producer.join();
  std::cout << "Exited gracefully" << std::endl;
  return 0;
}