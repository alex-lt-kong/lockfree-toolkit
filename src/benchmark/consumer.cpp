#include "../interprocess/spsc-queue-impl.h"
#include <iostream>
#include <thread>

using namespace RingBuffer;

int main() {
  auto q = Interprocess::SpscQueue("asdf123", true, (18 + 4) * 2);
  std::string bytes;
  while (true) {
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (!q.dequeue_impl(bytes)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      // std::cout << "q.dequeue() failed\n";
      continue;
    }
    std::cout << bytes << "\n";
  }
  std::cout << "Exited gracefully\n";
  return 0;
}