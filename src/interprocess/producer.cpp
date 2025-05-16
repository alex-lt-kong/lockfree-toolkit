//#include "spsc-queue-impl.h"
#include "mpsc-queue-impl.h"

#include <iostream>
#include <thread>

using namespace RingBuffer;

int main() {
    auto q = Interprocess::MpscQueue("asdf", false, 1000, 32);
    q.init();
    std::string bytes = "Hello world!";
    for (int i = 0; i < 110000; ++i) {
        auto payload = bytes + std::to_string(i);
        if (!q.enqueue_impl(payload)) {
            std::cout << "Enqueue() failed at " << i << "\n";
            --i;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    std::cout << "Exited gracefully\n";
    return 0;
}
