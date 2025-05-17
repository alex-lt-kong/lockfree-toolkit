# lockfree-toolkit

- Implementations of single-producer-single-consumer lock-free queues under one
  single interface:

    - A lock-free **intra**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)
    - A lock-free **inter**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)

## Performance

- `AMD Ryzen 5 PRO 6650U with Radeon Graphics`:
    - Intraprocess::SpscQueue:
      ```
      msg: 1000000000, throughput: 90.0M msg/sec
      msg: 1100000000, throughput: 90.1M msg/sec
      msg: 1200000000, throughput: 91.9M msg/sec
      msg: 1300000000, throughput: 91.0M msg/sec
      msg: 1400000000, throughput: 90.1M msg/sec
      msg: 1500000000, throughput: 89.5M msg/sec
      msg: 1600000000, throughput: 89.8M msg/sec
      msg: 1700000000, throughput: 91.9M msg/sec
      msg: 1800000000, throughput: 89.1M msg/sec
      msg: 1900000000, throughput: 89.8M msg/sec
      msg: 2000000000, throughput: 88.0M msg/sec
      ```