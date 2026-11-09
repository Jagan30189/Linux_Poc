# E-Commerce Order Processing POC

A Linux producer-consumer order system using POSIX message queues, named POSIX semaphores, `fork()`, and shared-memory statistics.

## Structure

- `include/`: public interfaces and the order model
- `src/`: producer, consumer, queue, semaphore, and logger implementations
- `src/order_system.c`: forks two producers and two consumers
- `src/monitor.c`: queue and statistics monitoring utility
- `src/stats_manager.c`: shared-memory counters protected by a named mutex semaphore
- `logs/orders.log`: append-only processing log

## Build

Requirements: Linux, GCC, GNU Make, and a mounted `/dev/mqueue` filesystem.

```sh
make
make clean-ipc
```

The queue has capacity 5. The `empty` and `full` named semaphores coordinate queue capacity and available work. A third named semaphore protects concurrent log writes. Shared memory tracks production, processing, fraud flags, recovery, failures, and inventory.

The consumer includes simulated service stages for fraud detection, order tracking through status logging, inventory synchronization, invalid-order validation, and one-step logging recovery. The `order_system` launcher uses `fork()` and `exec()` to run multiple producers and consumers.

## Run

The total number of orders consumed must equal the total number produced. Clean stale IPC objects before each new run. The fork-based complete example is:

```sh
make run-example
cat logs/orders.log
```

For interactive manual entry, start one producer and one consumer. Enter the same order count for both processes:

```sh
make clean-ipc
mkdir -p logs
: > logs/orders.log
./producer manual 1 & producer_pid=$!
./consumer 1 1 & consumer_pid=$!
wait $producer_pid $consumer_pid
```

The manual producer prompts for order ID, customer name, product ID, quantity, amount, and priority. Running `./producer` without arguments displays a menu for manual entry, automatic generation, or exit. The `manual` command accepts `<producer-id> <order-count>` after the mode name; the example above enters one order.

The producer syntax is `./producer <producer-id> <order-count>`. The consumer syntax is `./consumer <consumer-id> <order-count>`. Producer IDs must be unique. For a single-process pair, the defaults are ten orders:

```sh
make clean-ipc
./producer & producer_pid=$!
./consumer & consumer_pid=$!
wait $producer_pid $consumer_pid
```

Automatic producers generate priorities 1, 2, and 3. POSIX message queues deliver higher-priority messages first. Each consumer exits after its requested count. Run `./monitor` while a producer or consumer is active to view queue depth and shared statistics.

## Cleanup

```sh
make clean-ipc
make clean
```

## Tests

GoogleTest and Valgrind tests can be run with:

```sh
make gtest
make valgrind
```

The test target requires GoogleTest development headers and libraries (for
example, the `libgtest-dev` package on Debian-based Linux distributions).

Static analysis and coverage are available with:

```sh
make cppcheck
make misra
make coverage
```

`cppcheck` checks the C production sources. `misra` runs Cppcheck's MISRA C:2012
addon and returns nonzero when violations are found. `coverage` runs the tests
with GCC instrumentation and writes gcov reports under `coverage/`.

`clean-ipc` removes the named queue and semaphores. Run it after an interrupted process or before starting a new run.