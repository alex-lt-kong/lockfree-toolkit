#ifndef INTERPROCESS_SPSC_BETA_QUEUE_IMPL_H
#define INTERPROCESS_SPSC_BETA_QUEUE_IMPL_H

#include "../ringbuffer-interface.h"

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace RingBuffer::Interprocess {
  class SpscQueueBeta : public IRingBuffer<SpscQueueBeta, std::string> {
  private:
    static constexpr int FLAG_WRAPPED = -1;
    static constexpr int m_header_size = sizeof(int) * 2;
    int m_queue_size;
    // const int m_max_msg_size;
    // int m_max_element_size;
    char *m_base_ptr = nullptr;
    bool m_ownership;
    std::string m_mapped_file_name;
    size_t m_total_size = 0;
    std::unique_ptr<boost::interprocess::shared_memory_object> m_shm_obj;
    std::unique_ptr<boost::interprocess::mapped_region> m_region;

  public:
    explicit SpscQueueBeta(const std::string &queue_name,
                           const bool ownership = false,
                           const int queue_size_bytes = 1000)
      : m_ownership(ownership), m_mapped_file_name(queue_name) {
      m_queue_size = queue_size_bytes;
      // header + queue payload area.
      m_total_size = m_header_size + m_queue_size;
      // Use Boost.Interprocess to open (or create) and map the memory.
      namespace bip = boost::interprocess;

      m_shm_obj = std::make_unique<bip::shared_memory_object>(
        bip::open_or_create, m_mapped_file_name.c_str(), bip::read_write);
      // Resize the shared memory object.
      m_shm_obj->truncate(static_cast<long>(m_total_size));
      // Map the entire shared memory object into the process's address space.
      m_region =
          std::make_unique<bip::mapped_region>(*m_shm_obj, bip::read_write);
      m_base_ptr = static_cast<char *>(m_region->get_address());

      // If this process "owns" the queue, initialize it.
      if (m_ownership) {
        std::memset(m_base_ptr, 0, m_total_size);
      }
    }

    // Disable copy operations.
    SpscQueueBeta(const SpscQueueBeta &) = delete;

    SpscQueueBeta &operator=(const SpscQueueBeta &) = delete;

    ~SpscQueueBeta() { dispose(); }

    // Enqueues a message.
    // we cant std::move() in this case
    template<typename U>
      requires std::assignable_from<std::string &, U>
    bool enqueue_impl(U &&msg_bytes) {
      const int msg_length = static_cast<int>(msg_bytes.size());
      const int element_length = sizeof(int) + msg_length;
      const auto head_ptr = reinterpret_cast<int *>(m_base_ptr);
      const auto tail_ptr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
      // i.e. the base address of data segment
      char *data_base = m_base_ptr + m_header_size;

      const std::atomic_ref head_atomic(*head_ptr);
      const std::atomic_ref tail_atomic(*tail_ptr);
      // for head_atomic.load(), std::memory_order_relaxed works on x86 but breaks
      // on ARM64
      std::atomic_thread_fence(std::memory_order_acquire);
      const int head = head_atomic.load(std::memory_order_relaxed);
      const int tail = tail_atomic.load(std::memory_order_relaxed);

      const int used = get_used_bytes(head, tail);
      if (const int free = m_queue_size - used;
        free < element_length + msg_length) {
        return false;
      }

      int msg_offset = tail;
      // If the message record would not fit contiguously, write a wrap marker.
      if (msg_offset + element_length > m_queue_size) {
        if (m_queue_size - msg_offset >= static_cast<int>(sizeof(int))) {
          *reinterpret_cast<int *>(data_base + msg_offset) = FLAG_WRAPPED;
        }
        msg_offset = 0;
      }

      // Write data length field then the data itself. Note that these two writes
      // are not atomic
      *reinterpret_cast<int *>(data_base + msg_offset) = msg_length;
      // Write the payload first.
      std::memcpy(data_base + msg_offset + sizeof(int), msg_bytes.data(),
                  msg_length);
      /*
        const std::atomic_ref length_atomic(
            *reinterpret_cast<int *>(data_base + msg_offset));
        length_atomic.store(msg_length, std::memory_order_release);*/

      // Update the tail pointer, moving it by element_length.
      int new_tail = msg_offset + element_length;
      if (new_tail >= m_queue_size)
        new_tail = 0;
      std::atomic_thread_fence(std::memory_order_release);
      tail_atomic.store(new_tail, std::memory_order_relaxed);

      return true;
    }

    bool dequeue_impl(std::string &buffer) const {
      const auto head_ptr = reinterpret_cast<int *>(m_base_ptr);
      const auto tail_ptr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
      // i.e. the base address of data segment
      char *queue_base = m_base_ptr + m_header_size;

      const std::atomic_ref head_atomic(*head_ptr);
      const std::atomic_ref tail_atomic(*tail_ptr);
      std::atomic_thread_fence(std::memory_order_acquire);
      int head = head_atomic.load(std::memory_order_relaxed);
      // for tail_atomic.load(), std::memory_order_relaxed works on x86 but breaks
      // on ARM64
      if (const int tail = tail_atomic.load(std::memory_order_relaxed);
        head == tail) {
        return false; // Queue is empty, no message available.
      }

      int msg_length = *reinterpret_cast<int *>(queue_base + head);

      // Handle wrap marker.
      if (head + sizeof(msg_length) >= m_queue_size ||
          /*head + sizeof(msg_length) + msg_length >= m_queue_size ||*/
          msg_length == FLAG_WRAPPED) {
        head = 0;
        //    head_atomic.store(head, std::memory_order_release);
        msg_length = *reinterpret_cast<int *>(queue_base + head);
      }

      // Ensure the provided buffer is large enough.
      if (buffer.size() < static_cast<size_t>(msg_length)) {
        buffer.resize(msg_length);
      }
      std::memcpy(buffer.data(), queue_base + head + sizeof(int), msg_length);

      // Advance the head pointer.
      int new_head = head + static_cast<int>(sizeof(msg_length)) + msg_length;
      if (new_head >= m_queue_size)
        new_head = 0;
      std::atomic_thread_fence(std::memory_order_release);
      head_atomic.store(new_head, std::memory_order_relaxed);
      return true;
    }

    // Returns the number of used bytes in the queue. If head or tail is not
    // provided, they are re-read.
    [[nodiscard]] int get_used_bytes(int head = -1, int tail = -1) const {
      if (head == -1) {
        const auto head_ptr = reinterpret_cast<int *>(m_base_ptr);
        const std::atomic_ref head_atomic(*head_ptr);
        head = head_atomic.load(std::memory_order_acquire);
      }
      if (tail == -1) {
        const auto tail_ptr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
        const std::atomic_ref tail_atomic(*tail_ptr);
        tail = tail_atomic.load(std::memory_order_relaxed);
      }
      if (tail >= head)
        return tail - head;
      return m_queue_size - (head - tail);
    }

    [[nodiscard]] int head_impl() const {
      const auto head_ptr = reinterpret_cast<int *>(m_base_ptr);
      const std::atomic_ref head_atomic(*head_ptr);
      return head_atomic.load(std::memory_order_relaxed);
    }

    [[nodiscard]] int tail_impl() const {
      const auto tail_ptr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
      const std::atomic_ref tail_atomic(*tail_ptr);
      return tail_atomic.load(std::memory_order_relaxed);
    }

    void dispose() {
      m_region.reset();
      m_shm_obj.reset();
      if (m_ownership) {
        // Remove the shared memory object.
        boost::interprocess::shared_memory_object::remove(
          m_mapped_file_name.c_str());
      }
      m_base_ptr = nullptr;
    }
  };
} // namespace RingBuffer::Interprocess
#endif // INTERPROCESS_SPSC_BETA_QUEUE_IMPL_H
