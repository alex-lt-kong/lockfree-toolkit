#ifndef INC_SPSC_RINGBUFFER_IMPL_H
#define INC_SPSC_RINGBUFFER_IMPL_H

#include <atomic>
#include <vector>

/* Refer to
 * - https://github.com/facebook/folly/blob/main/folly/ProducerConsumerQueue.h
 * -
 * https://github.com/cameron314/readerwriterqueue/blob/master/readerwritercircularbuffer.h
 */

/* Notes:
 * - load() always comes with std::memory_order_acquire, aka, read-acquire
 * - store() always comes with std::memory_order_release, aka, write-release
 * - read-acquire and write-release are logical memory barriers
 * - A read-acquire guarantees that all following code starts after the
 * acquiring read
 * - A write-release guarantees that all preceding code completes before the
 * releasing write
 */
namespace LockFree {
template <typename T> class RingBufferSPSC {
private:
  /*
    Head/tail could be confusing, usually for FIFO queue, head is when
    element comes out, tail is where element gets in. To visualize it,
    you see that without wrapping around, Head is on the left while Tail
    is on the right:
            +----+----+----+----+----+----+
            |    |  B |  C |  D |    |    |
            +----+----+----+----+----+----+
                   ↑          ↑
                  Head       Tail
   * */
  const size_t m_capacity;
  std::vector<T> m_buffer;
  std::atomic<size_t> m_write_ptr; // Points to the NEXT available position to
                                   // write, i.e., the tail end of the queue
  std::atomic<size_t> m_read_ptr;  // Points to the NEXT available position to
                                   // read, i.e., the head end of the queue
public:
  // we want to distinguish between buffer empty (tail == head) and buffer
  // full (tail + 1 == head), so we need the allocate capacity+1
  explicit RingBufferSPSC(const size_t capacity)
      : m_capacity(capacity + 1), m_buffer(capacity + 1), m_write_ptr(0),
        m_read_ptr(0) {}

  // We need to define a new type U to make enqueue() work for lvalue
  // a new type U makes it a "forwarding reference" (a.k.a. "universal
  // reference"):
  // https://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers
  // Buy T&& is not a universal reference while U&& is?
  // T&& is NOT a universal reference because T is already a fixed type (not
  // deduced). U&& IS a universal reference because U is deduced dynamically.
  template <typename U>
  // Conversion (std::convertible_to<U, T>) creates a new object, so no
  // reference is needed. Assignment (std::assignable_from<T&, U>) modifies an
  // existing object, so T& is required. requires std::convertible_to<U, T>
    requires std::assignable_from<T &, U>
  bool enqueue(U &&item) {
    // What does `std::memory_order_relaxed` mean?
    // It means that we don't need to synchronize with other threads.
    auto tail = m_write_ptr.load(std::memory_order_relaxed);
    auto next_tail = tail + 1;
    if (next_tail == m_capacity) {
      next_tail = 0;
    }

    if (next_tail == m_read_ptr.load(std::memory_order_acquire)) {
      // tail + 1 == head , i.e., buffer is full
      return false;
    }

    m_buffer[tail] = std::forward<U>(item);
    m_write_ptr.store(next_tail, std::memory_order_release);
    return true;
  }

  bool dequeue(T &item) {
    size_t head = m_read_ptr.load(std::memory_order_relaxed);
    if (head == m_write_ptr.load(std::memory_order_acquire)) {
      // head == tail, i.e., buffer is empty
      return false;
    }

    auto next_head = head + 1;
    if (next_head == m_capacity) {
      next_head = 0;
    }
    item = std::move(m_buffer[head]);
    m_read_ptr.store(next_head, std::memory_order_release);
    return true;
  }

  [[nodiscard]] std::size_t size_approx() const {
    const size_t tail = m_write_ptr.load(std::memory_order_acquire);
    const size_t head = m_read_ptr.load(std::memory_order_acquire);
    if (tail >= head)
      return tail - head;
    return (m_capacity + tail - head) % m_capacity;
  }

  [[nodiscard]] std::size_t capacity() const { return m_capacity - 1; }
};
} // namespace LockFree

#endif // INC_SPSC_RINGBUFFER_IMPL_H