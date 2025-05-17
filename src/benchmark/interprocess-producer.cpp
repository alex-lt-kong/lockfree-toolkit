#include "../interprocess/spsc-queue-impl.h"
#include "utils.h"

#include <csignal>
#include <iostream>
#include <thread>

using namespace RingBuffer;

template <typename TImpl, typename T>
void producer_func_(IRingBuffer<TImpl, T> &q) {
  using namespace std::chrono;
  uint64_t msg = 1;
  std::string raw_msg = "hello";

  while (!ev_flag) {
    // raw_msg.assign(reinterpret_cast<const char *>(&msg), sizeof(msg));
    if (q.enqueue(raw_msg))
      msg++;
    // std::cout << "msg: " << msg << std::endl;
  }
}

int main() {
  if (signal(SIGINT, handle_signal) == SIG_ERR ||
      signal(SIGTERM, handle_signal) == SIG_ERR) {
    perror("signal()");
    return EXIT_FAILURE;
  }

  auto q = Interprocess::SpscQueue("asdf", false);
  q.init();
  producer_func(q);
  std::cout << "Exited gracefully\n";
  return 0;
}
