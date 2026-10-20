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
	--passengers 100 \
	--requests 100 \
	--workers 4 \
	--queue-capacity 16 \
	--output-dir artifacts
```

The program creates one timestamped run directory containing:

```text
events.log          one result line per request
reservations.csv    confirmed seat allocations
metrics.csv         machine-readable run metrics
summary.txt         human-readable result summary
```

## OOM Testing

`--fail-after N` forces tracked allocation failures after allocation attempt `N`:

```bash
./build/tatkal_poc --seats 10 --requests 1000 --workers 8 --fail-after 20
```

The booking path releases a seat when reservation allocation fails. The output records failed requests, OOM failures, rollback events, latency, peak tracked bytes, and outstanding allocations at report time.

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
make debug
make cppcheck
make valgrind
```