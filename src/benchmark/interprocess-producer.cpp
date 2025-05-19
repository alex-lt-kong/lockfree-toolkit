#include "../interprocess/spsc-queue-impl.h"
#include "../interprocess/spsc-queue-beta-impl.h"
#include "utils.h"

#include <csignal>
#include <iostream>

using namespace RingBuffer;

using SpscQueueImpl = Interprocess::SpscQueueBeta;
// using SpscQueueImpl = Interprocess::SpscQueue;

int main() {
  if (signal(SIGINT, handle_signal) == SIG_ERR ||
      signal(SIGTERM, handle_signal) == SIG_ERR) {
    perror("signal()");
    return EXIT_FAILURE;
  }

  auto q = SpscQueueImpl("test", false, 171);
  producer_func(q);
  std::cout << "Exited gracefully\n";
  return 0;
}
