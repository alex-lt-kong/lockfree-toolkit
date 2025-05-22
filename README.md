# lockfree-toolkit

- Implementations of single-producer-single-consumer lock-free queues under one
  single interface:

    - A lock-free **intra**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)
    - A lock-free **inter**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)

## Performance

- x86 has some of the strongest memory order among common architectures. Many
  subtle issues won't expose on it. Therefore, we intentionally run the queue on
  a wide variety of architectures + compilers

### x86_64

- `Intel(R) Core(TM) i7-14700` + `MSVC 14.43.34808`
    - Intraprocess::SpscQueue:
      ```
      msg: 33430000000, throughput: 284.46M msg/sec
      msg: 34920000000, throughput: 296.58M msg/sec
      msg: 36350000000, throughput: 285.71M msg/sec
      ```

    - Interprocess::SpscQueue:
      ```
      msg: 350000000, throughput: 45.60M msg/sec
      msg: 580000000, throughput: 45.39M msg/sec
      msg: 810000000, throughput: 45.51M msg/sec

      ```

- `AMD Ryzen 5 PRO 6650U` + `gcc 14.2.0`
    - Intraprocess::SpscQueue:
      ```    
      msg: 1930000000, throughput: 101.3M msg/sec
      msg: 2450000000, throughput: 103.6M msg/sec
      msg: 2970000000, throughput: 102.7M msg/sec
      ```

    - Interprocess::SpscQueue:
      ```
      msg: 726000000, throughput: 32.2M msg/sec
      msg: 759000000, throughput: 32.4M msg/sec
      msg: 792000000, throughput: 32.5M msg/sec   
      ```

### aarch64

- `Ampere Altra Max M128-30` (vCPU) + `clang 18.1.3`

    - Intraprocess::SpscQueue:
      ```
      msg: 110000000, throughput: 18.6M msg/sec
      msg: 210000000, throughput: 19.4M msg/sec
      msg: 290000000, throughput: 14.4M msg/sec
      ```

    - Interprocess::SpscQueue:
      ```
      msg: 1380000000, throughput: 10.04M msg/sec
      msg: 1440000000, throughput: 10.68M msg/sec
      msg: 1500000000, throughput: 11.28M msg/sec
      ```

- `Cortex-A53` + `gcc 12.2.0`

    - Intraprocess::SpscQueue:
      ```
      msg: 2450000000, throughput: 25.80M msg/sec
      msg: 2580000000, throughput: 25.87M msg/sec
      msg: 2710000000, throughput: 25.82M msg/sec
      ```

    - Interprocess::SpscQueue:
      ```
      msg: 170000000, throughput: 8.70M msg/sec
      msg: 220000000, throughput: 8.71M msg/sec
      msg: 270000000, throughput: 8.70M msg/sec
      ```

## Notes

1. On x86 (including x86-64), atomic_thread_fence functions issue no CPU
   instructions and only affect compile-time code motion, except for `std::
   atomic_thread_fence(std::memory_order_seq_cst)`. <sup>[[std::atomic_thread_fence]](https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence)</sup>

2. On strongly-ordered systems — x86, SPARC TSO, IBM mainframe, etc. —
   release-acquire ordering is automatic for the majority of operations. No
   additional CPU instructions are issued for this synchronization mode; only
   certain compiler optimizations are affected (e.g., the compiler is prohibited
   from moving non-atomic stores past the atomic store-release or performing
   non-atomic loads earlier than the atomic load-acquire). On weakly-ordered
   systems (ARM, Itanium, PowerPC), special CPU load or memory fence
   instructions are
   used. <sup>[[std::memory_order]](https://en.cppreference.com/w/cpp/atomic/memory_order#Release-Acquire_ordering)</sup>