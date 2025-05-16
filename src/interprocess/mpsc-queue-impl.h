#ifndef INTERPROCESS_MPSC_QUEUE_IMPL_H
#define INTERPROCESS_MPSC_QUEUE_IMPL_H

#include "spsc-queue-impl.h"  // Make sure this path matches your project structure.
#include <atomic>
#include <cstring>
#include <stdexcept>
#include <string>

namespace RingBuffer::Interprocess {

    // The Multiple-Producer-Single-Consumer queue derives from SpscQueue.
    class MpscQueue : public SpscQueue {
    private:
        int m_max_producer_count;

    protected:
        // We re-declare FLAG_WRAPPED since the base class keeps it private.
        static constexpr int FLAG_WRAPPED = -1;

    public:
        // Constructor:
        //   queueName       : Shared memory name (or file name) for the queue.
        //   ownership       : Whether this process owns and thus initializes the queue.
        //   capacity        : Lower bound on the maximum number of messages.
        //   maxMsgSize      : Maximum message payload size in bytes.
        //   max_producer_count: Maximum number of concurrent producers.
        MpscQueue(const std::string &queueName,
                  bool ownership = false,
                  int capacity = 1000,
                  int maxMsgSize = 128,
                  int max_producer_count = 8)
                : SpscQueue(queueName, ownership, capacity, maxMsgSize),
                  m_max_producer_count(max_producer_count) {
            // Adjust the queue size for multiple producers:
            // In C#, QueueSize is set to capacity * (MaxElementSize + max_producer_count + 1).
            m_queue_size =
                    capacity * (m_max_element_size + m_max_producer_count + 1);
        }

        // Enqueues a message. Here the message contents are in the std::string msgBytes.
        // (The message is not taken by ownership; you may reuse the message buffer after a failed enqueue.)
        template<typename U>
        requires std::assignable_from<std::string &, U>
        bool enqueue_impl(U &msgBytes) {
            int msgLength = static_cast<int>(msgBytes.size());
            if (msgLength > m_max_msg_size) {
                throw std::invalid_argument(
                        "Message length exceeds max element size");
            }

            int *head_ptr = reinterpret_cast<int *>(m_base_ptr);
            int *tail_ptr = reinterpret_cast<int *>(m_base_ptr + sizeof(int));
            char *data_offset = m_base_ptr + m_header_size;

            // Use atomic_ref for volatile-style semantics on the head pointer.
            std::atomic_ref<int> headAtomic(*head_ptr);
            int head = headAtomic.load(std::memory_order_acquire);

            std::atomic_ref<int> tailAtomic(*tail_ptr);
            int tail = 0;
            int newTail = 0;

            // Reserve space by advancing the tail pointer with an atomic CAS.
            while (true) {
                tail = tailAtomic.load(std::memory_order_acquire);
                // Compute used bytes: if tail >= head then used = tail - head; otherwise, wrap-around has occurred.
                /*int used = (tail >= head) ? (tail - head) : (
                        (m_queue_size - head) + tail);*/
                int used = getUsedBytes(head, tail);
                int free = m_queue_size - used;
                // Ensure there is always sufficient free space for _every_ producer to reserve a full slot.
                if (free < m_max_element_size * (m_max_producer_count + 1)) {
                    return false;
                }

                // If advancing by one slot would exceed the queue capacity, write a wrap marker and wrap to the beginning.
                if (tail + m_max_element_size >= m_queue_size) {
                    if (m_queue_size - tail >= static_cast<int>(sizeof(int))) {
                        *reinterpret_cast<int *>(data_offset +
                                                 tail) = FLAG_WRAPPED;
                    }
                    std::cout
                            << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n========== wrapped ==========\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                            << std::endl;
                    std::this_thread::sleep_for(
                            std::chrono::milliseconds(1000));
                    newTail = m_max_element_size; // Funny, newTail will be MaxElementSize
                } else {
                    newTail = tail + m_max_element_size;
                }
                // Attempt to reserve the space using CAS.
                if (tailAtomic.compare_exchange_weak(tail, newTail,
                                                     std::memory_order_acq_rel,
                                                     std::memory_order_acquire)) {
                    break;  // Successfully reserved the slot.
                }
                // Otherwise, loop and try again.
            }

            int msg_offset = tail;
            // Check for wrap-around: if the reserved slot overruns end of queue.
            if (msg_offset + m_max_element_size > m_queue_size) {
                if (m_queue_size - msg_offset >=
                    static_cast<int>(sizeof(int))) {
                    *reinterpret_cast<int *>(data_offset +
                                             msg_offset) = FLAG_WRAPPED;
                }
                msg_offset = 0;
            }

            // Write the message payload. It is placed after an int-sized header slot reserved for the message length.
            std::memcpy(data_offset + msg_offset + sizeof(int), msgBytes.data(),
                        msgLength);
            // Finally, write the length field. This write must become visible only after the payload is copied.
            *reinterpret_cast<int *>(data_offset + msg_offset) = msgLength;

            return true;
        }
    };

} // namespace RingBuffer::Interprocess

#endif //INTERPROCESS_MPSC_QUEUE_IMPL_H
