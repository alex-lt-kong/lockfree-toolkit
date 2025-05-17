#include "../interprocess/spsc-queue-impl.h"
#include "utils.h"

#include <csignal>
#include <iostream>
#include <thread>

using namespace RingBuffer;
template <typename TImpl, typename T>
void consumer_func_(IRingBuffer<TImpl, T> &q) {
  using namespace std::chrono;
  auto t0 = duration_cast<milliseconds>(system_clock::now().time_since_epoch())
                .count();
  uint64_t prev_msg = 0;
  while (!ev_flag) {
    uint64_t msg;
    std::string msg_str;
    if (!q.dequeue(msg_str))
      continue;

    /*
    std::memcpy(&msg, msg_str.data(), sizeof(uint64_t));
    if (prev_msg + 1 != msg) {
      std::cerr << "Unexpected message id: " << msg << std::endl;
    }
    prev_msg = msg;
    if (msg % 100'000'001 != 0)
      continue;
    const auto t1 =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count();
    if (t1 - t0 < 1000)
      continue;
    std::cout << "msg: " << msg
              << ", throughput: " << (msg / 1'000'000) / ((t1 - t0) / 1000)
              << "M msg/sec\n";*/
    std::cout << msg_str << std::endl;
  }
}

int main() {

  if (signal(SIGINT, handle_signal) == SIG_ERR ||
      signal(SIGTERM, handle_signal) == SIG_ERR) {
    perror("signal()");
    return EXIT_FAILURE;
  }

  auto q = Interprocess::SpscQueue("asdf", true);
  q.init();
  consumer_func(q);
  std::cout << "Exited gracefully\n";
  return 0;
}
