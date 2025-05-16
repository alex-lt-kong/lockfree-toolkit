
#ifndef INTERPROCESS_SPSC_QUEUE_IMPL_H
#define INTERPROCESS_SPSC_QUEUE_IMPL_H

#include "../ringbuffer-interface.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <atomic>

namespace RingBuffer::Interprocess {

    class SpscQueue : public RingBuffer::IRingBuffer<SpscQueue, std::string> {
    private:
        static constexpr int FLAG_MSG_UNCOMMITTED = -2;
        static constexpr int FLAG_WRAPPED = -1;
    protected:
        static constexpr int m_header_size = sizeof(int) * 2;
        int m_queue_size;
        const int m_max_msg_size;
        int m_max_element_size;
        char *m_base_ptr = nullptr;
        bool m_ownership;
        std::string m_mapped_file_name;
        size_t m_total_size = 0;
        std::unique_ptr<boost::interprocess::shared_memory_object> m_shm_obj;
        std::unique_ptr<boost::interprocess::mapped_region> m_region;

    public:
        SpscQueue(const std::string &queueName,
                  bool ownership = false,
                  int capacity = 1000, int maxMsgSize = 128)
                : m_max_msg_size(maxMsgSize),
                  m_ownership(ownership),
                  m_mapped_file_name(queueName) {
            // Compute MaxElementSize: message length field plus payload,
            // then align to a 4-byte boundary.
            m_max_element_size = m_max_msg_size + sizeof(int);
            int remainder = m_max_element_size % sizeof(int);
            if (remainder != 0) {
                m_max_element_size += (sizeof(int) - remainder);
            }
            // QueueSize is defined as capacity * (MaxElementSize + 1)
            m_queue_size = capacity * (m_max_element_size + 1);
        }

        // Disable copy operations.
        SpscQueue(const SpscQueue &) = delete;

        SpscQueue &operator=(const SpscQueue &) = delete;

        virtual ~SpscQueue() {
            dispose();
        }


        void init_impl() {
            // Total size = header + queue payload area.
            m_total_size = m_header_size + m_queue_size;
            int rem = static_cast<int>(m_total_size % sizeof(int));
            if (rem != 0) {
                m_total_size += (sizeof(int) -
                                 rem); // Align totalSize to 4 bytes.
            }

            // Use Boost.Interprocess to open (or create) and map the memory.
            namespace bip = boost::interprocess;
            try {
                m_shm_obj = std::make_unique<bip::shared_memory_object>(
                        bip::open_or_create,
                        m_mapped_file_name.c_str(),
                        bip::read_write
                );
                // Resize the shared memory object.
                m_shm_obj->truncate(m_total_size);
                // Map the entire shared memory object into the process's address space.
                m_region = std::make_unique<bip::mapped_region>(*m_shm_obj,
                                                                bip::read_write);
                m_base_ptr = static_cast<char *>(m_region->get_address());
            } catch (const std::exception &ex) {
                throw std::runtime_error(std::string(
                        "Boost shared memory initialization failed: ") +
                                         ex.what());
            }

            // If this process "owns" the queue, initialize it.
            if (m_ownership) {
                int *intPtr = reinterpret_cast<int *>(m_base_ptr);
                int numberOfIntegers = static_cast<int>(m_total_size /
                                                        sizeof(int));
                for (int i = 0; i < numberOfIntegers; i++) {
                    intPtr[i] = FLAG_MSG_UNCOMMITTED;
                }
                intPtr[0] = 0;  // head initialized to 0.
                intPtr[1] = 0;  // tail initialized to 0.
            }
        }

        // Enqueues a message. msgBytes is not taken by ownership.
        template<typename U>
        requires std::assignable_from<std::string &, U>
        bool enqueue_impl(U &msgBytes) {

            std::cout << "enqueue_impl() @ spsc called!" << std::endl;
            int msgLength = static_cast<int>(msgBytes.size());
            if (msgLength > m_max_msg_size) {
                throw std::invalid_argument(
                        "Message length exceeds max message size");
            }

            int *head_ptr = reinterpret_cast<int *>(m_base_ptr);
            int *tail_ptr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
            char *data_offset = m_base_ptr + m_header_size;

            std::atomic_ref<int> headAtomic(*head_ptr);
            std::atomic_ref<int> tailAtomic(*tail_ptr);
            int head = headAtomic.load(std::memory_order_relaxed);
            int tail = tailAtomic.load(std::memory_order_relaxed);

            int used = getUsedBytes(head, tail);
            int free = m_queue_size - used;
            if (free < m_max_element_size * 2)
                return false;

            int msg_offset = tail;
            // If the message record would not fit contiguously, write a wrap marker.
            if (msg_offset + m_max_element_size > m_queue_size) {
                if (m_queue_size - msg_offset >=
                    static_cast<int>(sizeof(int))) {
                    *reinterpret_cast<int *>(data_offset +
                                             msg_offset) = FLAG_WRAPPED;
                }
                msg_offset = 0;
            }

            // Write the payload first.
            std::memcpy(data_offset + msg_offset + sizeof(int), msgBytes.data(),
                        msgLength);
            // Then write the length field (with release semantics).
            std::atomic_ref<int> lengthAtomic(
                    *reinterpret_cast<int *>(data_offset + msg_offset));
            lengthAtomic.store(msgLength, std::memory_order_release);

            // Update the tail pointer, moving by the full slot size.
            int newTail = msg_offset + m_max_element_size;
            if (newTail >= m_queue_size) newTail = 0;
            tailAtomic.store(newTail, std::memory_order_release);

            return true;
        }

        // Dequeues a message. The message is copied into 'buffer'. The function returns
        // the message length, or -1 if no new message is available.
        bool dequeue_impl(std::string &buffer) {
            int *headPtr = reinterpret_cast<int *>(m_base_ptr);
            int *tailPtr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
            char *queueBase = m_base_ptr + m_header_size;

            std::atomic_ref<int> headAtomic(*headPtr);
            std::atomic_ref<int> tailAtomic(*tailPtr);
            int head = headAtomic.load(std::memory_order_relaxed);
            int tail = tailAtomic.load(std::memory_order_relaxed);

            if (head == tail)
                return false; // No message available.

            int msgLength = *reinterpret_cast<int *>(queueBase + head);
            // Handle wrap marker.
            if (msgLength == FLAG_WRAPPED) {
                std::cout
                        << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n========== wrapped ==========\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                head = 0;
                headAtomic.store(head, std::memory_order_release);
                msgLength = *reinterpret_cast<int *>(queueBase + head);
            }

            if (msgLength == FLAG_MSG_UNCOMMITTED)
                return false; // Slot allocated but not committed.

            // Mark the slot as uncommitted.
            *reinterpret_cast<int *>(queueBase + head) = FLAG_MSG_UNCOMMITTED;

            // Ensure the provided buffer is large enough.
            if (buffer.size() < static_cast<size_t>(msgLength)) {
                buffer.resize(msgLength);
            }
            std::memcpy(buffer.data(), queueBase + head + sizeof(int),
                        msgLength);

            // Advance the head pointer.
            int newHead = head + m_max_element_size;
            headAtomic.store(newHead, std::memory_order_release);
            return true;
        }

        // Returns the number of used bytes in the queue. If head or tail is not provided, they are re-read.
        int getUsedBytes(int head = -1, int tail = -1) const {
            if (head == -1) {
                int *headPtr = reinterpret_cast<int *>(m_base_ptr);
                std::atomic_ref<int> headAtomic(*headPtr);
                head = headAtomic.load(std::memory_order_relaxed);
            }
            if (tail == -1) {
                int *tailPtr = reinterpret_cast<int *>(m_base_ptr +
                                                       sizeof(int));
                std::atomic_ref<int> tailAtomic(*tailPtr);
                tail = tailAtomic.load(std::memory_order_relaxed);
            }
            if (tail >= head)
                return tail - head;
            return m_queue_size - (head - tail);
        }

        int head_impl() const {
            int *headPtr = reinterpret_cast<int *>(m_base_ptr);
            std::atomic_ref<int> headAtomic(*headPtr);
            return headAtomic.load(std::memory_order_relaxed);
        }

        int tail_impl() const {
            int *tailPtr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
            std::atomic_ref<int> tailAtomic(*tailPtr);
            return tailAtomic.load(std::memory_order_relaxed);
        }

        void dispose() {
            m_region.reset();
            m_shm_obj.reset();
            if (m_ownership) {
                // Remove the shared memory object.
                boost::interprocess::shared_memory_object::remove(
                        m_mapped_file_name.c_str());
                std::cout << "This process owns the queue ["
                          << m_mapped_file_name
                          << "], releasing resources." << std::endl;
            } else {
                std::cout << "This process does NOT own the queue ["
                          << m_mapped_file_name
                          << "], releasing resources." << std::endl;
            }
            m_base_ptr = nullptr;
        }
    };

} // namespace IpcPcQueue::Queue
#endif //INTERPROCESS_SPSC_QUEUE_IMPL_H
