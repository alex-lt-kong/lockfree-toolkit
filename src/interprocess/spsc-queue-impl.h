#ifndef INTERPROCESS_SPSC_QUEUE_IMPL_H
#define INTERPROCESS_SPSC_QUEUE_IMPL_H

#include "../ringbuffer-interface.h"

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace RingBuffer::Interprocess {

class SpscQueue : public RingBuffer::IRingBuffer<SpscQueue, std::string> {
private:
  // static constexpr int FLAG_MSG_UNCOMMITTED = -2;
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
  SpscQueue(const std::string &queue_name, const bool ownership = false,
            const int queue_size_bytes = 1000)
      : m_ownership(ownership), m_mapped_file_name(queue_name) {
    // m_max_element_size: message length field plus payload,
    // then align to a 4-byte boundary.
    // m_max_element_size = m_max_msg_size + sizeof(int);
    // m_max_element_size += sizeof(int) - m_max_element_size % sizeof(int);
    m_queue_size = queue_size_bytes;
  }

  // Disable copy operations.
  SpscQueue(const SpscQueue &) = delete;
  SpscQueue &operator=(const SpscQueue &) = delete;

  ~SpscQueue() { dispose(); }

  void init_impl() {
    // header + queue payload area.
    m_total_size = m_header_size + m_queue_size;
    // m_total_size must be aligned to 4 bytes
    // m_total_size += sizeof(int) - m_total_size % sizeof(int);

    // Use Boost.Interprocess to open (or create) and map the memory.
    namespace bip = boost::interprocess;
    try {
      m_shm_obj = std::make_unique<bip::shared_memory_object>(
          bip::open_or_create, m_mapped_file_name.c_str(), bip::read_write);
      // Resize the shared memory object.
      m_shm_obj->truncate(m_total_size);
      // Map the entire shared memory object into the process's address space.
      m_region =
          std::make_unique<bip::mapped_region>(*m_shm_obj, bip::read_write);
      m_base_ptr = static_cast<char *>(m_region->get_address());
    } catch (const std::exception &ex) {
      throw std::runtime_error(
          std::string("Boost shared memory initialization failed: ") +
          ex.what());
    }

    // If this process "owns" the queue, initialize it.

    if (m_ownership) {
      std::memset(m_base_ptr, 0, m_total_size);
      /*
      int *intPtr = reinterpret_cast<int *>(m_base_ptr);
      int numberOfIntegers = static_cast<int>(m_total_size / sizeof(int));
      for (int i = 0; i < numberOfIntegers; i++) {
        intPtr[i] = 0;
      }
      intPtr[0] = 0; // head initialized to 0.
      intPtr[1] = 0; // tail initialized to 0.*/
    }
  }

  // Enqueues a message. msgBytes is not taken by ownership.
  template <typename U>
    requires std::assignable_from<std::string &, U>
  bool enqueue_impl(U &msg_bytes) {
    const int msg_length = static_cast<int>(msg_bytes.size());
    const int element_length = sizeof(int) + msg_length;
    const auto head_ptr = reinterpret_cast<int *>(m_base_ptr);
    const auto tail_ptr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
    // i.e. the base address of data segment
    char *data_base = m_base_ptr + m_header_size;

    const std::atomic_ref head_atomic(*head_ptr);
    const std::atomic_ref tail_atomic(*tail_ptr);
    const int head = head_atomic.load(std::memory_order_relaxed);
    const int tail = tail_atomic.load(std::memory_order_relaxed);

    const int used = get_used_bytes(head, tail);
    const int free = m_queue_size - used;
    if (free < element_length * 2)
      return false;

    int msg_offset = tail;
    // If the message record would not fit contiguously, write a wrap marker.
    if (msg_offset + element_length > m_queue_size) {
      if (m_queue_size - msg_offset >= static_cast<int>(sizeof(int))) {
        *reinterpret_cast<int *>(data_base + msg_offset) = FLAG_WRAPPED;
      }
      msg_offset = 0;
    }

    // Write the payload first.
    std::memcpy(data_base + msg_offset + sizeof(int), msg_bytes.data(),
                msg_length);

    // Then write the length field (with release semantics).
    const std::atomic_ref length_atomic(
        *reinterpret_cast<int *>(data_base + msg_offset));
    length_atomic.store(msg_length, std::memory_order_release);

    // Update the tail pointer, moving by the full slot size.
    int new_tail = msg_offset + element_length;
    if (new_tail >= m_queue_size)
      new_tail = 0;
    tail_atomic.store(new_tail, std::memory_order_release);

    return true;
  }

  // Dequeues a message. The message is copied into 'buffer'. The function
  // returns the message length, or -1 if no new message is available.
  bool dequeue_impl(std::string &buffer) const {
    const auto head_ptr = reinterpret_cast<int *>(m_base_ptr);
    const auto tail_ptr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
    // i.e. the base address of data segment
    char *queue_base = m_base_ptr + m_header_size;

    const std::atomic_ref head_atomic(*head_ptr);
    const std::atomic_ref tail_atomic(*tail_ptr);
    int head = head_atomic.load(std::memory_order_relaxed);
    const int tail = tail_atomic.load(std::memory_order_relaxed);

    if (head == tail) {
      return false; // Queue is empty, no message available.
    }

    int msg_length = *reinterpret_cast<int *>(queue_base + head);

    // Handle wrap marker.
    if (head + sizeof(msg_length) >= m_queue_size ||
        /*head + sizeof(msg_length) + msg_length >= m_queue_size ||*/
        msg_length == FLAG_WRAPPED) {
      head = 0;
      head_atomic.store(head, std::memory_order_release);
      msg_length = *reinterpret_cast<int *>(queue_base + head);
    }

    // Ensure the provided buffer is large enough.
    if (buffer.size() < static_cast<size_t>(msg_length)) {
      buffer.resize(msg_length);
    }
    std::memcpy(buffer.data(), queue_base + head + sizeof(int), msg_length);

    // Advance the head pointer.
    int new_head = head + sizeof(msg_length) + msg_length;
    if (new_head >= m_queue_size)
      new_head = 0;
    // std::cout << "head: " << head << ", new_head: " << new_head << std::endl;
    head_atomic.store(new_head, std::memory_order_release);
    return true;
  }

  // Returns the number of used bytes in the queue. If head or tail is not
  // provided, they are re-read.
  int get_used_bytes(int head = -1, int tail = -1) const {
    if (head == -1) {
      const auto head_ptr = reinterpret_cast<int *>(m_base_ptr);
      const std::atomic_ref head_atomic(*head_ptr);
      head = head_atomic.load(std::memory_order_relaxed);
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

  int head_impl() const {
    const auto head_ptr = reinterpret_cast<int *>(m_base_ptr);
    const std::atomic_ref head_atomic(*head_ptr);
    return head_atomic.load(std::memory_order_relaxed);
  }

  int tail_impl() const {
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
      std::cout << "This process owns the queue [" << m_mapped_file_name
                << "], releasing resources." << std::endl;
    } else {
      std::cout << "This process does NOT own the queue [" << m_mapped_file_name
                << "], releasing resources." << std::endl;
    }
    m_base_ptr = nullptr;
  }
};

} // namespace RingBuffer::Interprocess
#endif // INTERPROCESS_SPSC_QUEUE_IMPL_H
