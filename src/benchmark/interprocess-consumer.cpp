#include "../interprocess/spsc-queue-impl.h"
#include "utils.h"

#include <csignal>
#include <iostream>
#include <thread>

using namespace RingBuffer;

int main() {

  if (signal(SIGINT, handle_signal) == SIG_ERR ||
      signal(SIGTERM, handle_signal) == SIG_ERR) {
    perror("signal()");
    return EXIT_FAILURE;
  }

  auto q = Interprocess::SpscQueue("asdf", true, 1'000'000, 8);
  q.init();
  consumer_func(q);
  std::cout << "Exited gracefully\n";
  return 0;
}
