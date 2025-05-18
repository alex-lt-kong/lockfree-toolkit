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
      msg: 460000000, throughput: 91.9M msg/sec
      msg: 910000000, throughput: 88.7M msg/sec
      msg: 1360000000, throughput: 89.1M msg/sec
      msg: 1810000000, throughput: 89.7M msg/sec
      msg: 2300000000, throughput: 97.3M msg/sec
      msg: 2790000000, throughput: 96.3M msg/sec
      msg: 3270000000, throughput: 95.9M msg/sec
      msg: 3760000000, throughput: 96.2M msg/sec
      msg: 4260000000, throughput: 98.3M msg/sec
      msg: 4740000000, throughput: 95.9M msg/sec
      msg: 5230000000, throughput: 97.9M msg/ses
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