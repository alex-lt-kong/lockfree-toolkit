
#include "utils.h"

#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

using namespace RingBuffer;

constexpr size_t q_size = INT16_MAX;

int main() {
  if (signal(SIGINT, handle_signal) == SIG_ERR ||
      signal(SIGTERM, handle_signal) == SIG_ERR) {
    perror("signal()");
    return EXIT_FAILURE;
  }

  Intraprocess::SpscQueue<uint64_t> q{1'000'000};
  std::thread thread_consumer(
      consumer_func<Intraprocess::SpscQueue<uint64_t>, uint64_t>, std ::ref(q));
  std::thread thread_producer(
      producer_func<Intraprocess::SpscQueue<uint64_t>, uint64_t>, std::ref(q));

  thread_consumer.join();
  thread_producer.join();
  std::cout << "Exited gracefully" << std::endl;
  return 0;
}