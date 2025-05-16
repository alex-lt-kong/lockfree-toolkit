# lockfree-toolkit

- Implementations of single-producer-single-consumer lock-free queues under one
  single interface:

    - A lock-free **intra**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)
    - A lock-free **inter**process Single-Producer-Single-Consumer bounded
      queue (ring buffer)

## Performance

- Intraprocess::SpscQueue: roughly 100 million 8-byte message per second