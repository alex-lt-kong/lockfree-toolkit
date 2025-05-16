#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <cstdint>

namespace RingBuffer {
struct Payload {
  uint64_t id;
  // int64_t unix_epoch_time_us;
  // char message[64 - sizeof(uint64_t) - sizeof(int64_t)];
};

} // namespace RingBuffer

#endif // PAYLOAD_H
