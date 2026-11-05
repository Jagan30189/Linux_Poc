# E-Commerce Order Processing POC

A Linux producer-consumer implementation using POSIX message queues and named POSIX semaphores.

## Structure

- `include/`: public interfaces and the order model
- `src/`: producer, consumer, queue, semaphore, and logger implementations
- `logs/orders.log`: append-only processing log

## Build

Requirements: Linux, GCC, GNU Make, and a mounted `/dev/mqueue` filesystem.

```sh
make
make clean-ipc
```

The queue has capacity 5. The `empty` and `full` named semaphores coordinate queue capacity and available work. A third named semaphore protects concurrent log writes.

## Run

The total number of orders consumed must equal the total number produced. Clean stale IPC objects before each new run:

```sh
mkdir -p logs
: > logs/orders.log
./producer 1 5 & producer_one=$!
./producer 2 5 & producer_two=$!
./consumer 1 5 & consumer_one=$!
./consumer 2 5 & consumer_two=$!
wait $producer_one $producer_two $consumer_one $consumer_two
```

The producer syntax is `./producer <producer-id> <order-count>`. The consumer syntax is `./consumer <consumer-id> <order-count>`. Producer IDs must be unique. For a single-process pair, the defaults are ten orders:

```sh
make clean-ipc
./producer & producer_pid=$!
./consumer & consumer_pid=$!
wait $producer_pid $consumer_pid
```

The producer generates priorities 1, 2, and 3. POSIX message queues deliver higher-priority messages first. Each consumer exits after its requested count, and the last consumer performs IPC name cleanup. `make run-example` runs the two-producer/two-consumer example.

## Cleanup

```sh
make clean-ipc
make clean
```

`clean-ipc` removes the named queue and semaphores. Run it after an interrupted process or before starting a new run.