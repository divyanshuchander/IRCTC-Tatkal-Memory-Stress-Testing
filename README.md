# IRCTC Tatkal Memory Stress Testing

A small C11/POSIX-threads proof of concept for measuring memory behavior during concurrent seat-booking load.

## Build

```bash
make debug
make cppcheck
```

## Run

```bash
./build/tatkal_poc \
	--seats 10 \
	--requests 100 \
	--workers 4 \
	--output-dir artifacts
```

Add `--verbose` to print each booking event and a system-memory sample every
500 milliseconds while the run is active:

```bash
./build/tatkal_poc --requests 1000 --workers 8 --verbose
```

The program creates one timestamped run directory containing:

```text
events.log          one result line per request
reservations.csv    confirmed seat allocations
metrics/metrics.csv machine-readable run metrics
metrics/system.csv sampled process memory and thread metrics
metrics/memory.log allocation counters at report time
metrics/oom.log     allocation-related failure events
summary.txt         human-readable result summary
```

The summary includes the run configuration, submitted/completed/unprocessed
requests, success and failure rates, seat totals, throughput, average and
maximum latency, and tracked allocation statistics. The CSV report contains
the same measurements as `metric,value` rows for scripts and spreadsheets.
Runtime logs are written through buffered, mutex-protected streams and flushed
when the run closes, avoiding a forced flush for every request.

## OOM Testing

`--fail-after N` forces tracked allocation failures after allocation attempt `N`:

```bash
./build/tatkal_poc --seats 10 --requests 1000 --workers 8 --fail-after 20
```

The booking path releases a seat when reservation allocation fails. The output
records request and reservation OOM failures separately, rollback events,
latency, peak tracked bytes, and outstanding allocations at report time.

The queue capacity is derived from the worker count, and passenger IDs are generated from request IDs. These implementation details do not need separate command-line flags.

## Source Modules

The implementation intentionally has a small number of modules:

```text
src/main.c       run orchestration and worker lifecycle
src/config.c     command-line configuration
src/memory.c     tracked allocation and deterministic OOM injection
src/queue.c      bounded FIFO request queue
src/booking.c    seat allocation, reservations, workers, cleanup
src/output.c     logs, reservation CSV, metrics, and summary files
```

The MVP uses one train and generated passengers. It focuses on bounded concurrent load, memory ownership, OOM behavior, and measurable output rather than railway business features.

## Validation

```bash
make clean
make build
make test
make coverage
make cppcheck
make valgrind
```

`make test` compiles `gtest/functionalTesting.cpp` with the C modules under
test and runs the GoogleTest executable. The executable and intermediate test
artifacts are ignored by Git. `make runTest` remains as a compatibility alias.
`make coverage` runs the functional tests with GCC coverage instrumentation
and prints line and branch coverage for the C modules in the terminal.

The simple monitoring mode reads Linux process values from `/proc/self/status`.
It is intended for observing a run, not for replacing Valgrind or measuring
production infrastructure.