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

  auto q = Interprocess::SpscQueue("test", false, 171);
  q.init();
  producer_func(q);
  std::cout << "Exited gracefully\n";
  return 0;
}
