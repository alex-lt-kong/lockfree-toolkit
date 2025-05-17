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

    - Interprocess::SpscQueue:
      ```
      msg: 726000000, throughput: 32.2M msg/sec
      msg: 759000000, throughput: 32.4M msg/sec
      msg: 792000000, throughput: 32.5M msg/sec
      msg: 824000000, throughput: 32.0M msg/sec
      msg: 856000000, throughput: 31.9M msg/sec
      msg: 889000000, throughput: 32.0M msg/sec
      msg: 922000000, throughput: 32.4M msg/sec
      msg: 955000000, throughput: 32.4M msg/sec
      msg: 988000000, throughput: 32.3M msg/sec
      ```