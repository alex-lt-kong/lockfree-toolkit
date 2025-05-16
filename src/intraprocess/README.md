# Lockfree SpscQueue

## A few interesting questions

- Q: What are `std::memory_order_relaxed`, `std::memory_order_acquire` and
  `std::memory_order_release`? Are they memory barriers?
- A:
    - Not really, they are C++'s memory ordering semantics. The memory orderings
      prevent the compiler and CPU from reordering memory
      accesses.<sup>[[Misunderstanding in Acquire/Release. Memory ordering](https://users.rust-lang.org/t/misunderstanding-in-acquire-release-memory-ordering/114010/4)]</sup>
    - whether a compiler needs to use any barrier instructions to implement the
      C++ semantics in the asm is just an implementation detail, quite separate
      from the correctness of the memory-order specified for
      operations.<sup>[[Reverse the [memory-barriers] [memory-order] synonym situation so questions about C++ std::memory_order can be tagged memory-order, not barriers](https://meta.stackoverflow.com/questions/419898/reverse-the-memory-barriers-memory-order-synonym-situation-so-questions-abou)]</sup>
    - Microsoft Copilot's summary: Traditional memory barriers (like `mfence` in
      x86 assembly) enforce explicit ordering at the hardware level, whereas C++
      atomic operations offer a higher-level abstraction that guides compiler
      optimizations while ensuring thread synchronization.


- Q: If we target x86 architecture only, is memory barrier needed?
- A: Strictly speaking the answer is no:
    - Among the commonly used architectures, x86-64 processors have the
      strongest memory order, but may still defer memory store instructions
      until after memory load
      instructions. <sup>[[Memory ordering](https://en.wikipedia.org/wiki/Memory_ordering)]</sup>
    - You don't need memory barriers to implement an SPSC queue for x86. You can
      do a relaxed store to the queue followed by a release write to
      producer_idx. As long as consumer begins with an acquire load from
      producer_idx it is guaranteed to see all stores to the queue memory before
      producer_idx, according to the happens before ordering. There are no
      memory barriers on x86 for acquire/release
      semantics. <sup>[[Avoiding expensive memory barriers](https://groups.google.com/g/mechanical-sympathy/c/tHe1ARCvxDU/m/QTvpMtTlAQAJ)]</sup>
    - One important point to remember here is that we are talking about
      instructions in program order that the compiler emitted during the
      compilation phase. The compiler itself is also free to reorder
      instructions in any way it sees fit to optimize your code! What this means
      is that it is not just the processor we need to worry about, but also the
      compiler. Using compiler memory barriers can help in ensuring
      this.<sup>[[Memory Barriers on x86](https://www.asrivas.me/blog/memory-barriers-on-x86/)]</sup>