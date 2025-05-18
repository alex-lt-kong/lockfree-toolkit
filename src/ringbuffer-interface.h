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

  ///
  /// @tparam U a dummy template to enable forwarding reference
  /// @param item data will be std::move()ed or copied from item to the queue
  /// @return true if item was enqueued, false if queue is full
  template <typename U>
    requires std::assignable_from<T &, U>
  bool enqueue(U &&item) {
    // TODO: make sure perfect forwarding works as expected
    return static_cast<TImpl *>(this)->enqueue_impl(std::forward<U>(item));
  }

  ///
  /// @param item data will be move()ed into item
  /// @return true if item was dequeued, false if queue is empty
  bool dequeue(T &item) {
    return static_cast<TImpl *>(this)->dequeue_impl(item);
  }

  int head() { return static_cast<TImpl *>(this)->head_impl(); }

  int tail() { return static_cast<TImpl *>(this)->tail_impl(); }
};

} // namespace RingBuffer

#endif // RINGBUFFER_INTERFACE_H
