#ifndef INTRAPROCESS_SPSC_QUEUE_BETA_IMPL_H
#define INTRAPROCESS_SPSC_QUEUE_BETA_IMPL_H

#include "../ringbuffer-interface.h"

#include <atomic>
#include <iostream>
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
namespace RingBuffer::Intraprocess {
  template<typename T>
  class SpscQueueBeta : public IRingBuffer<SpscQueueBeta<T>, T> {
  private:
    /*
      Head/tail could be confusing, usually for FIFO queue, head is when
      elements get dequeue()ed, tail is where elements gets enqueue()ed. To
     visualize it, you see that without wrapping around, Head is on the left while
     Tail is on the right:
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
    std::atomic<size_t> m_read_ptr; // Points to the NEXT available position to
    // read, i.e., the head end of the queue
  public:
    // we want to distinguish between buffer empty (tail == head) and buffer
    // full (tail + 1 == head), so we need the allocate capacity+1
    explicit SpscQueueBeta(const size_t capacity)
      : m_capacity(capacity + 1), m_buffer(capacity + 1), m_write_ptr(0),
        m_read_ptr(0) {
    }

    // We need to define a new type U to make enqueue() work for lvalue
    // a new type U makes it a "forwarding reference" (a.k.a. "universal
    // reference"):
    // https://isocpp.org/blog/2012/11/universal-references-in-c11-scott-meyers
    // But why T&& is not a forwarding reference while U&& is?
    // T&& is NOT a forwarding reference because T is already a fixed type (not
    // deduced). U&& IS a universal reference because U is deduced dynamically.
    template<typename U>
    // Conversion (std::convertible_to<U, T>) creates a new object, so no
    // reference is needed (i.e., we have T, not T&). Assignment
    // (std::assignable_from<T&, U>) modifies an existing object, so T& is
    // required.
      requires std::assignable_from<T &, U>
    bool enqueue_impl(U &&item) {
      // What does `std::memory_order_relaxed` mean?
      // It means that we don't need to synchronize with other threads.
      auto tail = m_write_ptr.load(std::memory_order_relaxed);
      auto next_tail = tail + 1;
      if (next_tail == m_capacity) {
        next_tail = 0;
      }
      std::atomic_thread_fence(std::memory_order_acquire);
      if (const size_t head = m_read_ptr.load(std::memory_order_relaxed); next_tail == head) {
        // tail + 1 == head , i.e., buffer is full
        return false;
      }

      m_buffer[tail] = std::forward<U>(item);

      std::atomic_thread_fence(std::memory_order_release);
      m_write_ptr.store(next_tail, std::memory_order_relaxed);
      // everything else the same, moving std::atomic_thread_fence() to below m_write_ptr.store() breaks the queue.
      return true;
    }

    bool dequeue_impl(T &item) {
      auto head = m_read_ptr.load(std::memory_order_relaxed);
      std::atomic_thread_fence(std::memory_order_acquire);
      if (const auto tail = m_write_ptr.load(std::memory_order_relaxed); head == tail) {
        // head == tail, i.e., buffer is empty
        return false;
      }

      auto next_head = head + 1;
      if (next_head == m_capacity) {
        next_head = 0;
      }
      item = std::move(m_buffer[head]);

      std::atomic_thread_fence(std::memory_order_release);
      m_read_ptr.store(next_head, std::memory_order_relaxed);
      // everything else the same, moving std::atomic_thread_fence() to above m_read_ptr.store() breaks the queue.
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

    [[nodiscard]] int head_impl() const {
      return m_read_ptr.load(std::memory_order_acquire);
    }

    [[nodiscard]] int tail_impl() const {
      return m_write_ptr.load(std::memory_order_acquire);
    }
  };
} // namespace RingBuffer::Intraprocess

#endif // INTRAPROCESS_SPSC_QUEUE_BETA_IMPL_H
