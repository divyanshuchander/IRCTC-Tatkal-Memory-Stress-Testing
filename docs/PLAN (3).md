# IRCTC Tatkal Booking Memory Stress Testing System

## Development Plan

**Project type:** Proof of Concept  
**Language:** C11  
**Environment:** Linux using GitHub Codespaces  
**Concurrency:** POSIX Threads  
**Build tool:** GNU Make  
**Validation:** GCC warnings, Cppcheck, Valgrind  

---

## 1. Project Overview

The IRCTC Tatkal Booking Memory Stress Testing System is a command-line C application that simulates many passengers attempting to reserve a limited number of train seats concurrently.

The project is not intended to reproduce the full IRCTC platform. Its purpose is to demonstrate and study:

- Concurrent request processing
- Race conditions and their prevention
- Mutex-protected seat allocation
- Dynamic memory ownership and cleanup
- Memory behavior under increasing workload
- Safe handling of allocation failures
- Preservation and recovery of confirmed reservations
- Basic performance and memory reporting

The application should remain small enough to understand. Each stage must produce a working program before the next capability is added.

---

## 2. Problem Being Solved

When several booking requests access the same seat inventory at the same time, two or more threads may observe the same seat as available and allocate it simultaneously. This produces duplicate bookings and inconsistent seat counts.

The application must prove that:

1. Multiple booking requests can run concurrently.
2. One seat can be assigned to only one passenger.
3. Shared data remains consistent.
4. Allocation failures do not corrupt confirmed bookings.
5. All dynamically allocated memory is released correctly.
6. The system can report what happened during a test run.

The central engineering problem is therefore not railway business functionality. It is **safe concurrent access to limited shared resources while memory and workload pressure increase**.

---

## 3. Project Objectives

### Primary Objectives

- Build a correct in-memory booking simulator.
- Process booking requests using POSIX threads.
- Prevent duplicate seat allocation using mutex synchronization.
- Manage dynamic memory safely.
- Validate the application using Valgrind and Cppcheck.
- Observe success, failure, latency, and memory behavior.

### Secondary Objectives

- Inject controlled allocation failures.
- Reject affected bookings gracefully during OOM conditions.
- Save confirmed reservations to a file.
- Recover reservation and seat state after restart.
- Run progressive stress scenarios.
- Produce a concise execution report.

### Non-Goals

The following are outside the PoC:

- Live IRCTC integration
- Payment processing
- Authentication
- Web or mobile interface
- Database integration
- Networking
- Distributed deployment
- Waiting list, RAC, fare calculation, or real train scheduling

---

## 4. Minimum Viable Product

The MVP is complete when the application can:

1. Create one train with a configurable number of seats.
2. Create multiple passengers.
3. Create multiple booking requests.
4. manage an input queue(FIFO) for the requests
5. Process requests using a small fixed worker pool.
6. Protect seat allocation with a per-train mutex.
7. Store confirmed reservations in a linked list.
8. Prevent duplicate seat allocation.
9. Print total, successful, and failed booking counts.
10. Print remaining seats and total execution time.
11. Release all allocated memory.
12. Pass Cppcheck and Valgrind validation.

### MVP Demonstration Scenario

```text
Train count:            1
Available seats:       10
Passengers:           100
Booking requests:     100
Worker threads:         4

Expected successful:   10
Expected failed:       90
Duplicate seats:        0
Remaining seats:        0
Memory leaks:           0
Invalid memory access:  0
```

Persistence, custom memory tracking, OOM injection, and advanced reporting are not part of the first MVP. They are added after concurrent booking is correct.

---

## 5. Feature Priorities

### Essential Features

- Train and seat creation
- Passenger creation
- Booking request creation
- Reservation creation
- Concurrent worker threads
- Mutex synchronization
- Duplicate-seat prevention
- Dynamic allocation and cleanup
- Error-code-based failure reporting
- Basic metrics
- Valgrind and Cppcheck validation

### Required Enhancements After MVP

- Application-level allocation metrics
- Deterministic OOM injection
- Graceful booking rollback
- Reservation-file persistence
- Restart recovery
- Progressive stress testing
- Text or CSV report generation

### Optional Features

- Multiple trains
- Configuration files
- Advanced logging levels
- Passenger and train input files
- Cancellation
- Seat preference
- JSON reports
- Coverage reports
- GitHub Actions
- Helgrind analysis

Optional features should be considered only after all required phases are stable.

---

## 6. Functional Requirements

### FR-01: Train Management

The system shall create at least one train with a unique ID, name, total-seat count, available-seat count, and seat array.

### FR-02: Passenger Management

The system shall create and store passengers with a unique ID, validated name, and age.

### FR-03: Booking Request Management

The system shall create booking requests containing request, passenger, and train identifiers.

### FR-04: Seat Allocation

The system shall allocate one available seat for a successful request and shall not allocate the same seat more than once.

### FR-05: Reservation Management

The system shall create a reservation for each confirmed booking and store confirmed reservations in a linked list.

### FR-06: Concurrent Processing

The system shall process booking requests using POSIX worker threads.

### FR-07: Synchronization

The system shall protect shared seat state using mutex synchronization.

### FR-08: Booking Result

The system shall return a clear success or failure result for every processed booking request.

### FR-09: Memory Management

The system shall check allocation results and release every owned allocation during normal and failed execution paths.

### FR-10: Monitoring

The system shall collect request, success, failure, seat, and execution-time metrics.

### FR-11: OOM Handling

After the MVP, the system shall support controlled allocation failure and reject the affected request without corrupting confirmed reservations.

### FR-12: Persistence and Recovery

After the MVP, the system shall persist confirmed reservations and restore reservation and seat state after restart.

### FR-13: Reporting

After the MVP, the system shall write a concise text or CSV execution report.

---

## 7. Non-Functional Requirements

### Correctness

- One seat must never be confirmed for two reservations.
- Available-seat count must remain consistent with seat state.
- Every accepted request must reach one final result.

### Reliability

- A failed request must not damage another confirmed booking.
- Partial initialization and partial processing must be cleaned up safely.

### Performance

- The worker count and request count must be configurable.
- Critical sections must be short.
- The design must avoid one global lock around the complete booking operation.

### Memory Safety

- All allocations must be checked.
- Array indexes must be validated.
- Freed pointers should be set to `NULL` where pointer-to-pointer cleanup APIs are used.
- Valgrind must report no invalid access or definite leaks at clean shutdown.

### Maintainability

- Public interfaces belong in header files.
- Each module has one clear responsibility.
- Ownership and thread-safety rules must be documented.
- Implementation details must remain private to source files where practical.

### Security and Coding Quality

- Validate command-line and API inputs.
- Avoid unsafe string functions.
- Check pthread and file-operation return values.
- Compile with strict warning flags.
- Review Cppcheck findings.
- Follow applicable MISRA C principles and document justified deviations.

### Portability

- Target Linux and C11.
- Keep Linux-specific behavior isolated when possible.

---

## 8. Technology Stack and Trade-Offs

### Selected Stack

- **GitHub Codespaces / Ubuntu:** reproducible Linux environment
- **C11:** explicit control over pointers and memory
- **GCC:** compiler and warning diagnostics
- **GNU Make:** simple build automation
- **POSIX Threads:** worker concurrency
- **pthread mutexes and condition variables:** synchronization
- **GDB:** runtime debugging
- **Valgrind Memcheck:** independent memory validation
- **Cppcheck:** static analysis
- **Git and GitHub:** version control
- **Local CSV/text file:** later persistence and reports

### Important Trade-Offs

#### C instead of C++

C makes allocation, ownership, pointer errors, and cleanup visible, which supports the project objectives. The trade-off is greater responsibility for manual resource management.

#### Worker pool instead of one thread per request

A worker pool gives controlled resource usage and can process many requests with a small number of threads. It requires a synchronized queue and condition variables, so it is introduced only after the single-threaded booking flow works.

#### Linked list for reservations

A linked list naturally demonstrates dynamic allocation and the required linked-list data structure. The trade-off is additional synchronization and cleanup complexity compared with an array.

#### In-memory MVP before file persistence

In-memory processing keeps the first milestone focused on booking correctness and synchronization. Persistence is added later because it introduces file validation, durability, rollback, and recovery concerns.

#### Make instead of CMake

GNU Make is sufficient for this small Linux C project. CMake would add configuration overhead without solving an immediate problem.

---

## 9. Architecture Overview

The application uses one process containing the main thread, a bounded request queue, and a small fixed worker pool.

```text
Command-Line Configuration
            |
            v
Application Controller
            |
            v
Passenger / Train Test Data
            |
            v
Booking Request Generator
            |
            v
Bounded Request Queue
            |
            v
POSIX Worker Pool
            |
            v
Booking Manager
       |           |
       v           v
Passenger Store   Train and Seat Manager
                         |
                         v
                 Reservation Store
                         |
                         v
                 Metrics and Summary
```

### Core Design Decisions

- Start with one train.
- Generate passengers and requests programmatically.
- Use command-line arguments for basic configuration.
- Use a bounded worker pool for concurrent execution.
- Use one mutex per train for seat state.
- Use one mutex for the reservation linked list.
- Use one mutex for composite metrics.
- Add persistence and custom memory tracking after the MVP.

---

## 10. Components and Responsibilities

### 10.1 Application Controller

Responsible for:

- Parsing configuration
- Initializing modules
- Creating test data
- Starting and stopping the workload
- Joining threads
- Printing final results
- Initiating cleanup

Likely files:

```text
src/main.c
include/app_config.h
src/app_config.c
include/system_context.h
```

### 10.2 Passenger Manager

Responsible for:

- Passenger validation
- Passenger creation
- Passenger storage
- Lookup by passenger ID
- Passenger cleanup

Likely files:

```text
include/passenger_manager.h
src/passenger_manager.c
```

### 10.3 Train and Seat Manager

Responsible for:

- Train initialization
- Seat-array ownership
- Available-seat count
- Mutex-protected seat allocation
- Seat rollback or release
- Train cleanup

Likely files:

```text
include/train_manager.h
src/train_manager.c
include/seat_allocation_manager.h
src/seat_allocation_manager.c
```

### 10.4 Booking Manager

Responsible for:

- Request validation
- Passenger and train lookup
- Seat-allocation coordination
- Reservation creation
- Rollback after a later failure
- Booking-result generation

Likely files:

```text
include/booking_manager.h
src/booking_manager.c
```

### 10.5 Reservation Store

Responsible for:

- Owning confirmed reservations
- Appending reservation nodes
- Preventing duplicate identifiers
- Traversal for validation and reports
- Linked-list cleanup

Likely files:

```text
include/reservation_store.h
src/reservation_store.c
```

### 10.6 Thread Manager

Responsible for:

- Bounded request queue
- Worker-thread creation
- Request submission
- Worker shutdown
- Worker joining
- Partial thread-creation failure cleanup

Likely files:

```text
include/thread_manager.h
src/thread_manager.c
```

### 10.7 Monitoring

Responsible for:

- Total requests
- Successful bookings
- Failed bookings
- Active and peak bookings
- Execution time
- Later latency and throughput metrics

Likely files:

```text
include/monitoring_manager.h
src/monitoring_manager.c
```

### 10.8 Post-MVP Reliability Components

Later phases introduce:

```text
memory_manager.h / .c
memory_leak_detector.h / .c
oom_handler.h / .c
recovery_manager.h / .c
memory_stress_generator.h / .c
report_generator.h / .c
```

These should not be created as empty placeholders before their phases begin.

---

## 11. Data Flow

### Initialization

```text
Command-line values
    -> validated configuration
    -> passenger store
    -> train and seats
    -> reservation store
    -> metrics
    -> request queue and workers
```

### Booking

```text
Generated request
    -> bounded queue
    -> worker thread
    -> Booking Manager
    -> passenger lookup
    -> train lookup
    -> mutex-protected seat allocation
    -> reservation creation
    -> reservation linked-list append
    -> metrics update
    -> final booking result
```

### Failure

```text
Validation or allocation failure
    -> reject affected request
    -> release temporary memory
    -> release seat if reservation was not committed
    -> record failure
    -> continue processing other requests
```

### Shutdown

```text
Producer finishes
    -> close queue
    -> workers finish remaining requests
    -> join workers
    -> capture final metrics
    -> print or write report
    -> free reservations
    -> free requests and passengers
    -> free train seats
    -> destroy mutexes and condition variables
    -> exit
```

---

## 12. Lean Project Structure

Create directories immediately, but add module files only when their phase begins.

```text
irctc-tatkal-memory-stress-poc/
├── .devcontainer/
│   ├── devcontainer.json
│   └── Dockerfile
├── .vscode/
│   ├── launch.json
│   ├── settings.json
│   └── tasks.json
├── config/
├── data/
├── docs/
│   ├── HLD/
│   └── LLD/
├── include/
├── logs/
├── reports/
├── scripts/
├── src/
│   └── main.c
├── tests/
│   ├── integration/
│   ├── stress/
│   └── unit/
├── .gitignore
├── Makefile
├── PLAN.md
└── README.md
```

---

# 13. Implementation Phases

## Phase 0: Development Environment

### Goal

Create a reproducible Codespaces environment and prove that the build and validation tools work.

### Tasks

#### Task 0.1: Create the repository and Codespace

**What to do**

- Create the GitHub repository.
- Open an Ubuntu-based Codespace.
- Add the project directories.

**Likely files/components**

```text
README.md
PLAN.md
.gitignore
```

**Prerequisite**

None.

**Verify**

```bash
pwd
git status
gcc --version
```

#### Task 0.2: Configure the development container

**What to do**

Install GCC, Make, GDB, Valgrind, Cppcheck, Git, and process-monitoring tools through the dev container.

**Likely files/components**

```text
.devcontainer/devcontainer.json
.devcontainer/Dockerfile
```

**Prerequisite**

Task 0.1.

**Verify**

Rebuild the Codespace, then run:

```bash
gcc --version
make --version
gdb --version
valgrind --version
cppcheck --version
```

#### Task 0.3: Create the initial build

**What to do**

- Add a minimal `main.c`.
- Add strict debug and release compiler settings.
- Add build, run, clean, Cppcheck, and Valgrind targets.

**Likely files/components**

```text
src/main.c
Makefile
.vscode/tasks.json
.vscode/launch.json
```

**Prerequisite**

Task 0.2.

**Verify**

```bash
make clean
make debug
make run
make cppcheck
make valgrind
```

### Phase Completion Criteria

- Codespace rebuilds successfully.
- Minimal application compiles with zero warnings.
- GDB launches the executable.
- Cppcheck and Valgrind execute successfully.

---

## Phase 1: Core Types and Domain Structures

### Goal

Define the data that flows through the application before implementing behavior.

### Learning Outcome

Understand how C structures model domain objects and how headers define contracts between modules.

### Tasks

#### Task 1.1: Define common types and errors

**What to do**

- Define the project ID type.
- Define booking status values.
- Define categorized error codes.
- Provide error-code-to-text conversion.

**Likely files/components**

```text
include/common_types.h
include/error_codes.h
src/error_codes.c
tests/unit/test_error_codes.c
```

**Prerequisite**

Phase 0.

**Verify**

- Every known error has readable text.
- Unknown values return a safe fallback.
- Unit test exits successfully.

#### Task 1.2: Define domain structures

**What to do**

Define minimal forms of:

- Passenger
- Seat
- Train
- BookingRequest
- BookingResult
- Reservation

Do not implement threads or persistence yet.

**Likely files/components**

```text
include/passenger_manager.h
include/train_manager.h
include/booking_manager.h
include/reservation_store.h
```

**Prerequisite**

Task 1.1.

**Verify**

- Headers compile together.
- No circular include dependency exists.
- Types use fixed-width integers where appropriate.

#### Task 1.3: Add basic command-line configuration

**What to do**

Support only essential values:

```text
--seats
--requests
--workers
```

Validate ranges and use simple defaults.

**Likely files/components**

```text
include/app_config.h
src/app_config.c
src/main.c
tests/unit/test_app_config.c
```

**Prerequisite**

Task 1.1.

**Verify**

```bash
./build/tatkal_poc --seats 10 --requests 100 --workers 4
./build/tatkal_poc --seats 0
```

The first command should accept the configuration. The second should fail clearly.

### Phase Completion Criteria

- Domain types are understandable and minimal.
- Invalid configuration is rejected.
- Headers compile with zero warnings.

---

## Phase 2: Single-Threaded Passenger and Train Management

### Goal

Create valid passengers, one train, and its seat inventory without concurrency.

### Learning Outcome

Understand allocation ownership, arrays, validation, and cleanup before shared access is introduced.

### Tasks

#### Task 2.1: Implement Passenger Manager

**What to do**

- Initialize a passenger store.
- Create validated passengers.
- Add passengers to the store.
- Find passengers by ID.
- Destroy the store and all owned passengers.

**Likely files/components**

```text
include/passenger_manager.h
src/passenger_manager.c
tests/unit/test_passenger_manager.c
```

**Prerequisite**

Phase 1.

**Verify**

- Valid passenger creation succeeds.
- Duplicate IDs are rejected.
- Empty and oversized names are rejected.
- Store-capacity overflow is rejected.
- Valgrind reports no leaks.

#### Task 2.2: Implement Train Manager

**What to do**

- Create one train.
- Dynamically allocate a contiguous seat array.
- Initialize external seat numbers from 1.
- Find the train by ID.
- Destroy train resources.

**Likely files/components**

```text
include/train_manager.h
src/train_manager.c
tests/unit/test_train_manager.c
```

**Prerequisite**

Phase 1.

**Verify**

- Zero seats are rejected.
- Every seat starts available.
- Available-seat count matches total seats.
- Cleanup is Valgrind-clean.

#### Task 2.3: Implement sequential seat allocation

**What to do**

- Allocate the first available seat.
- Update the available count.
- Release a valid allocated seat.
- Reject invalid release operations.

**Likely files/components**

```text
include/seat_allocation_manager.h
src/seat_allocation_manager.c
tests/unit/test_seat_allocation_manager.c
```

**Prerequisite**

Task 2.2.

**Verify**

- Every seat is allocated at most once.
- Full trains return a seat-unavailable error.
- Releasing a seat makes it available again.
- Counts remain correct after every operation.

### Phase Completion Criteria

- Passengers and train state are created and destroyed safely.
- Sequential seat allocation is correct.
- Valgrind reports no invalid access or leak.

---

## Phase 3: Single-Threaded Booking and Reservation Store

### Goal

Complete one end-to-end booking before adding pthreads.

### Learning Outcome

Understand orchestration, linked-list ownership, and rollback when a later operation fails.

### Tasks

#### Task 3.1: Implement the reservation linked list

**What to do**

- Initialize the reservation store.
- Append a fully initialized reservation node.
- Find a reservation by ID.
- Reject duplicate reservation IDs.
- Destroy every node.

**Likely files/components**

```text
include/reservation_store.h
src/reservation_store.c
tests/unit/test_reservation_store.c
```

**Prerequisite**

Phase 2.

**Verify**

- Head, tail, and count are correct.
- Empty, one-node, and multi-node cleanup work.
- Duplicate IDs are rejected.
- Valgrind is clean.

#### Task 3.2: Implement booking-request creation

**What to do**

- Validate request, passenger, and train IDs.
- Allocate and initialize a BookingRequest.
- Define request ownership and destruction.

**Likely files/components**

```text
include/booking_manager.h
src/booking_manager.c
tests/unit/test_booking_request.c
```

**Prerequisite**

Phase 1.

**Verify**

- Valid creation succeeds.
- Zero IDs and null output pointers are rejected.
- Request cleanup is leak-free.

#### Task 3.3: Implement single-threaded booking processing

**What to do**

The Booking Manager should:

1. Validate the request.
2. Find the passenger.
3. Find the train.
4. Allocate a seat.
5. Allocate and initialize a reservation.
6. Append the reservation.
7. Return a BookingResult.
8. Release the seat if a later step fails.

**Likely files/components**

```text
include/system_context.h
include/booking_manager.h
src/booking_manager.c
tests/integration/test_booking_flow.c
src/main.c
```

**Prerequisite**

Tasks 2.1, 2.3, 3.1, and 3.2.

**Verify**

- One valid request is confirmed.
- Unknown passenger is rejected.
- Unknown train is rejected.
- Full train is rejected.
- Injected reservation-creation failure returns the seat.
- Reservation can be found after success.

### Phase Completion Criteria

The program demonstrates:

```text
One train
Multiple passengers
One or more sequential booking requests
Confirmed reservations
Correct seat counts
Complete cleanup
```

Do not begin threading until this phase is stable.

---

## Phase 4: Concurrent Booking MVP

### Goal

Add POSIX worker threads and prove that seats cannot be allocated twice.

### Learning Outcome

Understand shared mutable state, mutex ownership, condition variables, worker lifecycle, and race-condition prevention.

### Tasks

#### Task 4.1: Protect shared train state

**What to do**

- Add a mutex owned by each train.
- Initialize and destroy it with the train.
- Lock only around seat search, allocation, release, and count updates.
- Check every pthread return value.

**Likely files/components**

```text
include/train_manager.h
src/train_manager.c
src/seat_allocation_manager.c
tests/unit/test_seat_allocation_manager.c
```

**Prerequisite**

Phase 3.

**Verify**

- All sequential tests still pass.
- Error paths release the lock.
- Train destruction safely destroys the mutex.

#### Task 4.2: Protect the reservation linked list

**What to do**

- Add a mutex owned by ReservationStore.
- Protect append and other mutable list operations.
- Avoid holding the reservation mutex while allocating memory.

**Likely files/components**

```text
include/reservation_store.h
src/reservation_store.c
```

**Prerequisite**

Task 3.1.

**Verify**

Run concurrent append tests and confirm:

- Count equals nodes traversed.
- Head and tail are valid.
- No node is lost or duplicated.

#### Task 4.3: Implement a bounded request queue

**What to do**

- Store pending BookingRequest pointers.
- Add a queue mutex.
- Add not-empty and not-full condition variables.
- Support enqueue, dequeue, close, and destroy.
- Define ownership transfer clearly.

**Likely files/components**

```text
include/thread_manager.h
src/thread_manager.c
tests/unit/test_request_queue.c
```

**Prerequisite**

Task 3.2.

**Verify**

- FIFO order is preserved.
- Producer blocks or follows the documented policy when full.
- Consumer waits when empty.
- Closing the queue wakes blocked threads.
- Every queued request is eventually owned or destroyed.

#### Task 4.4: Implement the fixed worker pool

**What to do**

- Create the configured number of workers.
- Workers dequeue requests and call Booking Manager.
- Close the queue after all requests are submitted.
- Join every worker.
- Clean up after partial thread-creation failure.

**Likely files/components**

```text
include/thread_manager.h
src/thread_manager.c
src/main.c
tests/integration/test_concurrent_booking.c
```

**Prerequisite**

Tasks 4.1, 4.2, and 4.3.

**Verify**

Use the core contention test:

```text
Seats:                 1
Concurrent requests: 100
Expected successful:   1
Expected failed:      99
Duplicate seat:        0
```

Repeat the test many times.

#### Task 4.5: Add basic thread-safe metrics

**What to do**

Record:

- Total processed
- Successful
- Failed
- Active bookings
- Peak active bookings
- Test start and end time

**Likely files/components**

```text
include/monitoring_manager.h
src/monitoring_manager.c
src/booking_manager.c
src/main.c
```

**Prerequisite**

Task 4.4.

**Verify**

- Total equals success plus failure.
- Active count ends at zero.
- Successful count does not exceed seats.
- Peak active count does not exceed worker count.

### MVP Completion Criteria

The MVP is complete when:

- Multiple workers process requests.
- No seat is duplicated.
- No deadlock occurs in repeated tests.
- Metrics are internally consistent.
- Every worker is joined.
- Cppcheck has no unresolved critical issue.
- Valgrind reports no invalid access or definite leak.

Stop here for an MVP review before adding the following enhancements.

---

## Phase 5: Memory Tracking and Controlled OOM

### Goal

Observe memory usage and prove that allocation failure does not corrupt booking state.

### Learning Outcome

Understand allocation metadata, ownership under failure, rollback, and the difference between application tracking and Valgrind.

### Tasks

#### Task 5.1: Define allocation ownership documentation

**What to do**

Document the owner and cleanup function for every allocated type:

- Passenger
- Passenger pointer array
- Train seat array
- BookingRequest
- ReservationNode
- Request queue storage
- Worker handle array

**Likely files/components**

```text
README.md
include/*.h
```

**Prerequisite**

MVP completion.

**Verify**

Every allocation site has one defined owner and cleanup path.

#### Task 5.2: Add tracked allocation metrics

**What to do**

Implement wrappers that record:

- Current bytes
- Peak bytes
- Allocation count
- Free count
- Failure count
- Outstanding allocations

Avoid recursive tracking of tracker metadata.

**Likely files/components**

```text
include/memory_manager.h
src/memory_manager.c
tests/unit/test_memory_manager.c
```

**Prerequisite**

Task 5.1.

**Verify**

- Allocate/free updates counters correctly.
- Peak bytes never fall.
- Outstanding allocation test is detected.
- Valgrind independently confirms cleanup.

#### Task 5.3: Add deterministic failure injection

**What to do**

Support safe failure modes:

- Fail next allocation
- Fail after N allocations
- Fail after tracked-byte threshold
- Disable injection

Do not create an unbounded memory-exhaustion loop.

**Likely files/components**

```text
include/oom_handler.h
src/oom_handler.c
src/memory_manager.c
tests/unit/test_oom_policy.c
```

**Prerequisite**

Task 5.2.

**Verify**

Each configured failure occurs at the expected allocation and normal behavior resumes after disabling it.

#### Task 5.4: Verify booking rollback under OOM

**What to do**

Inject failure after seat allocation but before reservation commit. The request must fail, and the seat must become available again.

**Likely files/components**

```text
src/booking_manager.c
tests/integration/test_oom_booking.c
```

**Prerequisite**

Task 5.3.

**Verify**

- Affected booking is rejected.
- Seat count is restored.
- Existing confirmed reservation remains unchanged.
- Metrics record the allocation failure.
- Other workers can continue according to policy.

### Phase Completion Criteria

- Memory metrics are accurate.
- Allocation failure is deterministic.
- Booking rollback is correct.
- Valgrind remains clean.

---

## Phase 6: Persistence and Recovery

### Goal

Save confirmed reservations and rebuild in-memory state after restart.

### Learning Outcome

Understand file validation, durable records, conflicting data, and state reconstruction.

### Tasks

#### Task 6.1: Define a versioned reservation record

**What to do**

Define a bounded CSV representation containing:

```text
version,reservation_id,request_id,train_id,passenger_id,seat_number,status,timestamp
```

Document valid ranges and maximum line length.

**Likely files/components**

```text
include/recovery_manager.h
docs/LLD/
```

**Prerequisite**

MVP completion.

**Verify**

Example valid and invalid records are documented and testable.

#### Task 6.2: Persist confirmed reservations

**What to do**

- Open the journal safely.
- Serialize append operations with a file mutex.
- Write confirmed records.
- Check write and flush results.
- Return file errors to the Booking Manager.

**Likely files/components**

```text
include/recovery_manager.h
src/recovery_manager.c
src/booking_manager.c
tests/integration/test_persistence.c
```

**Prerequisite**

Task 6.1.

**Verify**

- One successful booking produces one valid row.
- Failed bookings are not persisted.
- Concurrent confirmations produce complete, non-interleaved rows.

#### Task 6.3: Recover reservation and seat state

**What to do**

- Load records before workers start.
- Validate every field.
- Restore reservation nodes.
- Mark recovered seats allocated.
- Recalculate available seats.
- Skip and count invalid records.
- Reject duplicate reservation and seat conflicts.

**Likely files/components**

```text
src/recovery_manager.c
src/main.c
tests/integration/test_recovery.c
```

**Prerequisite**

Task 6.2.

**Verify**

- Valid restart restores reservations.
- Recovered seats cannot be booked again.
- Corrupt rows are reported without corrupting valid state.
- Missing file starts with empty reservation state.

### Phase Completion Criteria

- Confirmed reservations survive restart.
- Seat occupancy is reconstructed correctly.
- Invalid and conflicting file records are handled safely.

---

## Phase 7: Progressive Stress Testing and Reports

### Goal

Run bounded load stages and create useful evidence about booking, latency, and memory behavior.

### Learning Outcome

Understand the difference between functional correctness and measured behavior under increasing load.

### Tasks

#### Task 7.1: Add latency and throughput metrics

**What to do**

Use a monotonic clock to calculate:

- Per-request latency
- Average latency
- Maximum latency
- Total test duration
- Requests per second

**Likely files/components**

```text
include/monitoring_manager.h
src/monitoring_manager.c
src/booking_manager.c
tests/unit/test_monitoring_manager.c
```

**Prerequisite**

MVP completion.

**Verify**

- No completed requests avoids divide-by-zero.
- Average and max values are internally consistent.
- Metrics do not change booking correctness.

#### Task 7.2: Add bounded stress stages

**What to do**

Run configurable stages such as:

```text
Stage 1:  2 workers,  100 requests
Stage 2:  4 workers,  500 requests
Stage 3:  8 workers, 1000 requests
```

Every stage must respect configured safety limits.

**Likely files/components**

```text
include/memory_stress_generator.h
src/memory_stress_generator.c
tests/stress/test_progressive_load.c
```

**Prerequisite**

Tasks 5.2 and 7.1.

**Verify**

- All stages terminate.
- No duplicate seat occurs.
- Per-stage metrics are captured.
- Configured maximums are respected.

#### Task 7.3: Generate final text and CSV reports

**What to do**

Report:

- Configuration
- Total, successful, and failed requests
- Remaining seats
- Duration, average/max latency, throughput
- Current/peak memory and allocation failures
- Recovery summary

Generate from snapshots without holding locks during file output.

**Likely files/components**

```text
include/report_generator.h
src/report_generator.c
reports/
tests/integration/test_report_generator.c
```

**Prerequisite**

Tasks 5.2, 6.3, and 7.1.

**Verify**

- Text and CSV reports are created.
- Required fields are present.
- Unwritable paths return an error.
- Report generation does not modify runtime metrics.

### Phase Completion Criteria

- Progressive stages run within safe limits.
- Performance and memory behavior are visible.
- Reports are reproducible and readable.

---

## Phase 8: Quality Hardening and Demonstration

### Goal

Prepare the PoC for technical review.

### Tasks

#### Task 8.1: Execute full validation

**What to do**

Run all builds and tests.

**Likely files/components**

```text
Makefile
scripts/
tests/
```

**Prerequisite**

Required feature phases complete.

**Verify**

```bash
make clean
make debug
make test
make cppcheck
make valgrind
make stress-test
make release
```

#### Task 8.2: Review concurrency safety

**What to do**

Check:

- Lock ownership
- Lock order
- Every unlock path
- Queue closure
- Worker joining
- Partial startup failure
- Metrics consistency

**Likely files/components**

```text
src/thread_manager.c
src/train_manager.c
src/seat_allocation_manager.c
src/reservation_store.c
src/monitoring_manager.c
```

**Prerequisite**

Task 8.1.

**Verify**

Repeat the single-seat contention test at least 100 times without failure or hang.

#### Task 8.3: Review memory safety

**What to do**

Check every allocation, ownership transfer, rollback, and cleanup path.

**Likely files/components**

All source and header files that allocate or own resources.

**Prerequisite**

Task 8.1.

**Verify**

Valgrind acceptance:

```text
Invalid reads:          0
Invalid writes:         0
Invalid frees:          0
Definitely lost:        0 bytes
Indirectly lost:        0 bytes
```

#### Task 8.4: Prepare demonstration scenarios

**What to do**

Prepare commands and expected results for:

1. Normal booking
2. One-seat contention
3. Allocation failure and rollback
4. Restart recovery
5. Progressive stress run

**Likely files/components**

```text
README.md
config/
scripts/
```

**Prerequisite**

Tasks 8.1 to 8.3.

**Verify**

A reviewer can run each scenario using documented commands and understand the output.

### Project Completion Criteria

- Required functional requirements are demonstrated.
- No duplicate seat allocation occurs.
- No worker remains unjoined.
- No deadlock occurs in verification runs.
- Allocation-failure rollback preserves state.
- Confirmed reservations recover after restart.
- Cppcheck has no unresolved critical finding.
- Valgrind acceptance criteria pass.
- Reports and documentation match implemented behavior.

---

## 14. Testing Strategy

### Unit Tests

Test one module at a time:

- Validation
- Boundary values
- Ownership and cleanup
- Array indexes
- Linked-list operations
- Mutex-protected counters
- Failure paths

### Integration Tests

Test interactions:

- Passenger + Train + Booking
- Booking + Reservation Store
- Queue + Worker Pool + Booking
- Booking + OOM rollback
- Booking + Persistence
- Recovery + Train state
- Metrics + Report

### Concurrency Tests

Focus on invariants rather than only output:

- No duplicate seat number
- Successful bookings never exceed seats
- Total equals successful plus failed
- Reservation count equals successful bookings
- Available plus allocated equals total seats
- All workers terminate

### Memory Tests

Use both:

1. Application memory metrics
2. Valgrind as the independent validator

A custom tracker does not replace Valgrind.

### Stress Tests

Stress tests must be bounded. Start with small stages in Codespaces and increase only after observing safe resource use.

---

## 15. Known Risks

### Race Condition

Two workers may allocate the same seat.

**Mitigation:** Perform seat check and allocation within one train-mutex critical section.

### Deadlock

Locks may be acquired in inconsistent order.

**Mitigation:** Avoid nested locks where possible and document one lock order.

### Reservation Allocation Failure

A seat may be allocated before reservation memory is available.

**Mitigation:** Release the seat when reservation creation or commit fails.

### Memory Leak

Error paths may skip cleanup.

**Mitigation:** Explicit ownership, cleanup functions, focused failure tests, and Valgrind.

### Worker Shutdown Hang

Workers may remain blocked on an empty queue.

**Mitigation:** Queue-close state plus a condition-variable broadcast and cooperative shutdown.

### Corrupt Recovery File

Invalid rows may produce inconsistent seat state.

**Mitigation:** Validate every row before applying it and skip/report conflicts.

### Excessive Codespace Load

Unbounded stress may destabilize the environment.

**Mitigation:** Hard safety limits and deterministic OOM injection instead of uncontrolled exhaustion.

### Over-Engineering

The implementation may grow into a production railway system.

**Mitigation:** Keep the non-goals visible and add only features tied to a PoC objective.

---

## 16. Unresolved Issues and Decisions

These decisions should be settled when their phase begins:

- Exact queue capacity and behavior when full
- Whether multiple trains are required for the final demonstration
- Whether report output needs both text and CSV
- Whether a configuration file is needed after command-line options
- Exact journal durability policy: `fflush()` only or stronger Linux `fsync()` for demonstration
- Whether application-level leak tracking is required in addition to allocation metrics and Valgrind
- Maximum safe worker and request limits for the selected Codespace size
- Whether optional GitHub Actions should run build, tests, and Cppcheck

None of these decisions blocks the first three phases.

---

## 17. Progress Checklist

- [ ] Phase 0: Development environment
- [ ] Phase 1: Core types and domain structures
- [ ] Phase 2: Passenger, train, and sequential seat management
- [ ] Phase 3: Single-threaded booking and reservation store
- [ ] Phase 4: Concurrent booking MVP
- [ ] MVP review and approval
- [ ] Phase 5: Memory tracking and controlled OOM
- [ ] Phase 6: Persistence and recovery
- [ ] Phase 7: Progressive stress testing and reports
- [ ] Phase 8: Quality hardening and demonstration

---

## 18. Immediate Starting Point

Begin only with Phase 0.

The first work session should end with:

- A GitHub repository
- A working Codespace
- A minimal executable
- A debug build
- A working GDB launch
- Successful Cppcheck execution
- Successful Valgrind execution

Do not create the business modules until the development environment is verified. After Phase 0, implement Phase 1 and review the domain structures before proceeding.
