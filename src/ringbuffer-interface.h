#ifndef RINGBUFFER_INTERFACE_H
#define RINGBUFFER_INTERFACE_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace RingBuffer {

template <typename TImpl, typename T> class IRingBuffer {
public:
  ~IRingBuffer() = default;

  void init() { static_cast<TImpl *>(this)->init_impl(); }

  template <typename U>
    requires std::assignable_from<T &, U>
  bool enqueue(U &&item) {
    // TODO: make sure perfect forwarding works as expected
    return static_cast<TImpl *>(this)->enqueue_impl(std::forward<U>(item));
  }

  bool dequeue(T &item) {
    return static_cast<TImpl *>(this)->dequeue_impl(item);
  }

  int head() { return static_cast<TImpl *>(this)->head_impl(); }

  int tail() { return static_cast<TImpl *>(this)->tail_impl(); }

  // virtual int GetUsedBytes(int head = -1, int tail = -1) const = 0;
};

} // namespace RingBuffer

#endif // RINGBUFFER_INTERFACE_H
