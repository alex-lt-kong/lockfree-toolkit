# lockfree-toolkit

- Implementations of single-producer-single-consumer lock-free queues under one
  single interface:

    - A lock-free **intra**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)
    - A lock-free **inter**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)

## Performance

- `Intel(R) Core(TM) i7-14700` + `MSVC 14.43.34808`
    - Intraprocess::SpscQueue:
      ```
      msg: 33430000000, throughput: 284.46M msg/sec
      msg: 34920000000, throughput: 296.58M msg/sec
      msg: 36350000000, throughput: 285.71M msg/sec
      msg: 37780000000, throughput: 284.97M msg/sec
      msg: 39240000000, throughput: 290.55M msg/sec
      msg: 40650000000, throughput: 280.43M msg/sec
      ```

    - Interprocess::SpscQueue:
      ```
      msg: 350000000, throughput: 45.60M msg/sec
      msg: 580000000, throughput: 45.39M msg/sec
      msg: 810000000, throughput: 45.51M msg/sec
      msg: 1040000000, throughput: 45.46M msg/sec
      msg: 1270000000, throughput: 45.53M msg/sec
      ```

- `AMD Ryzen 5 PRO 6650U` + `gcc 14.2.0`:
    - Intraprocess::SpscQueue:
      ```    
      msg: 1930000000, throughput: 101.3M msg/sec
      msg: 2450000000, throughput: 103.6M msg/sec
      msg: 2970000000, throughput: 102.7M msg/sec
      msg: 3480000000, throughput: 101.1M msg/sec
      msg: 4000000000, throughput: 102.9M msg/sec
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