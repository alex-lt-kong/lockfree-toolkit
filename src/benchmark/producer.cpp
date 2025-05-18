#include "../interprocess/spsc-queue-impl.h"

#include <iostream>
#include <thread>

using namespace RingBuffer;

int main() {
  auto q = Interprocess::SpscQueue("asdf123", false, (18 + 4) * 2);
  const std::string bytes = "Hello world!";
  for (int i = 0; i < 110000; ++i) {
    auto payload = bytes + std::to_string(i);
    if (!q.enqueue_impl(payload)) {
      std::cout << "Enqueue() failed at " << i
                << ", payload.size(): " << payload.size() << "\n";
      --i;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  std::cout << "Exited gracefully\n";
  return 0;
}