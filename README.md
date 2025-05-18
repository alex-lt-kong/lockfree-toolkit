# lockfree-toolkit

- Implementations of single-producer-single-consumer lock-free queues under one
  single interface:

    - A lock-free **intra**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)
    - A lock-free **inter**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)

## Performance

- `AMD Ryzen 5 PRO 6650U` + `gcc 14.2.0`:
    - Intraprocess::SpscQueue:
      ```    
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
      ```

- `Ampere Altra Max M128-30` (vCPU) + `clang 18.1.3`

    - Intraprocess::SpscQueue:
      ```
      msg: 110000000, throughput: 18.6M msg/sec
      msg: 210000000, throughput: 19.4M msg/sec
      msg: 290000000, throughput: 14.4M msg/sec
      msg: 360000000, throughput: 12.5M msg/sec
      msg: 460000000, throughput: 17.9M msg/sec
      ```

    - Interprocess::SpscQueue:
      ```
      msg: 260000000, throughput: 7.8M msg/sec
      msg: 310000000, throughput: 8.6M msg/sec
      msg: 360000000, throughput: 8.9M msg/sec
      msg: 410000000, throughput: 8.9M msg/sec
      msg: 460000000, throughput: 8.8M msg/sec
      ```