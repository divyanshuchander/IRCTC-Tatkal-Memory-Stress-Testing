#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "booking.h"
#include "config.h"
#include "memory.h"
#include "output.h"
#include "queue.h"
}

struct QueueThreadData {
    request_queue *queue;
    request *request_value;
    request *result;
};

static void *push_request(void *argument)
{
    QueueThreadData *data = static_cast<QueueThreadData *>(argument);
    data->result = queue_push(data->queue, data->request_value) ? data->request_value : nullptr;
    return nullptr;
}

static void *pop_request(void *argument)
{
    QueueThreadData *data = static_cast<QueueThreadData *>(argument);
    data->result = queue_pop(data->queue);
    return nullptr;
}

TEST(ConfigParseTest, DefaultValues)
{
    char program[] = "test_config";
    char *argv[] = {program, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(1, argv, &parsed_config), 1);
    ASSERT_EQ(parsed_config.seats, 10);
    ASSERT_EQ(parsed_config.requests, 100);
    ASSERT_EQ(parsed_config.workers, 4);
    ASSERT_STREQ(parsed_config.output_dir, "artifacts");
}

TEST(ConfigParseTest, ParsesSeats)
{
    char program[] = "test_config";
    char option[] = "--seats";
    char value[] = "20";
    char *argv[] = {program, option, value, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(3, argv, &parsed_config), 1);
    ASSERT_EQ(parsed_config.seats, 20);
}

TEST(ConfigParseTest, ParsesRequests)
{
    char program[] = "test_config";
    char option[] = "--requests";
    char value[] = "50";
    char *argv[] = {program, option, value, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(3, argv, &parsed_config), 1);
    ASSERT_EQ(parsed_config.requests, 50);
}

TEST(ConfigParseTest, ParsesWorkers)
{
    char program[] = "test_config";
    char option[] = "--workers";
    char value[] = "2";
    char *argv[] = {program, option, value, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(3, argv, &parsed_config), 1);
    ASSERT_EQ(parsed_config.workers, 2);
}

TEST(ConfigParseTest, ParsesVerbose)
{
    char program[] = "test_config";
    char option[] = "--verbose";
    char *argv[] = {program, option, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(2, argv, &parsed_config), 1);
    ASSERT_TRUE(parsed_config.verbose);
}

TEST(ConfigParseTest, ParsesOutputDirectory)
{
    char program[] = "test_config";
    char option[] = "--output-dir";
    char value[] = "test-output";
    char *argv[] = {program, option, value, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(3, argv, &parsed_config), 1);
    ASSERT_STREQ(parsed_config.output_dir, "test-output");
}

TEST(ConfigParseTest, HelpReturnsTwo)
{
    char program[] = "test_config";
    char option[] = "--help";
    char *argv[] = {program, option, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(2, argv, &parsed_config), 2);
}

TEST(ConfigParseTest, RejectsUnknownOption)
{
    char program[] = "test_config";
    char option[] = "--unknown";
    char value[] = "10";
    char *argv[] = {program, option, value, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(3, argv, &parsed_config), 0);
}

TEST(ConfigParseTest, RejectsMissingValue)
{
    char program[] = "test_config";
    char option[] = "--seats";
    char *argv[] = {program, option, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(2, argv, &parsed_config), 0);
}

TEST(ConfigParseTest, RejectsZeroSeats)
{
    char program[] = "test_config";
    char option[] = "--seats";
    char value[] = "0";
    char *argv[] = {program, option, value, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(3, argv, &parsed_config), 0);
}

TEST(ConfigParseTest, RejectsNegativeSeats)
{
    char program[] = "test_config";
    char option[] = "--seats";
    char value[] = "-1";
    char *argv[] = {program, option, value, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(3, argv, &parsed_config), 0);
}

TEST(ConfigParseTest, RejectsInvalidFailAfter)
{
    char program[] = "test_config";
    char option[] = "--fail-after";
    char value[] = "-1";
    char *argv[] = {program, option, value, nullptr};
    config parsed_config{};

    ASSERT_EQ(config_parse(3, argv, &parsed_config), 0);
}

TEST(ConfigPrintUsageTest, PrintsUsage)
{
    testing::internal::CaptureStdout();
    config_print_usage("test_config");
    std::string output = testing::internal::GetCapturedStdout();

    ASSERT_NE(output.find("Usage:"), std::string::npos);
}

TEST(MemoryTest, NormalAllocation)
{
    void* ptr = memory_alloc(10);

    ASSERT_NE(ptr, nullptr);
    memory_free(ptr);
}

TEST(MemoryTest, ZeroSizeAllocation)
{
    ASSERT_EQ(memory_alloc(0), nullptr);
}

TEST(MemoryTest, MemoryCurrentBytes)
{
    void* ptr = memory_alloc(10);

    ASSERT_NE(ptr, nullptr);
    ASSERT_GE(memory_current_bytes(), static_cast<size_t>(10));
    memory_free(ptr);
}

TEST(MemoryTest, MemoryPeakBytes)
{
    void* ptr = memory_alloc(10);

    ASSERT_NE(ptr, nullptr);
    ASSERT_GE(memory_peak_bytes(), memory_current_bytes());
    memory_free(ptr);
}

TEST(MemoryTest, MemoryAllocations)
{
    size_t before = memory_allocations();
    void* ptr = memory_alloc(10);

    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ(memory_allocations(), before + 1);
    memory_free(ptr);
}

TEST(MemoryTest, MemoryFrees)
{
    size_t before = memory_frees();
    void* ptr = memory_alloc(10);

    ASSERT_NE(ptr, nullptr);
    memory_free(ptr);
    ASSERT_EQ(memory_frees(), before + 1);
}

TEST(MemoryTest, MemoryFailures)
{
    size_t before = memory_failures();
    memory_fail_after(0);

    ASSERT_EQ(memory_alloc(10), nullptr);
    ASSERT_EQ(memory_failures(), before + 1);
    memory_fail_after(-1);
}

TEST(MemoryTest, MemoryOutstanding)
{
    void* ptr = memory_alloc(10);

    ASSERT_NE(ptr, nullptr);
    memory_free(ptr);
    ASSERT_EQ(memory_outstanding(), 0);
}

TEST(MemoryTest, FreeNullPointer)
{
    memory_free(nullptr);
    SUCCEED();
}

TEST(QueueTest, InitializesQueue)
{
    request_queue queue{};

    ASSERT_TRUE(queue_init(&queue, 2));
    ASSERT_EQ(queue.capacity, 2);
    ASSERT_EQ(queue.count, 0);
    ASSERT_FALSE(queue.closed);
    queue_destroy(&queue);
}

TEST(QueueTest, RejectsAllocationFailure)
{
    request_queue queue{};

    memory_fail_after(0);
    ASSERT_FALSE(queue_init(&queue, 2));
    memory_fail_after(-1);
}

TEST(QueueTest, PushAndPopRequest)
{
    request_queue queue{};
    request request{1, 10};

    ASSERT_TRUE(queue_init(&queue, 2));
    ASSERT_TRUE(queue_push(&queue, &request));
    ASSERT_EQ(queue_pop(&queue), &request);
    queue_destroy(&queue);
}

TEST(QueueTest, PreservesRequestOrder)
{
    request_queue queue{};
    request first{1, 10};
    request second{2, 20};

    ASSERT_TRUE(queue_init(&queue, 2));
    ASSERT_TRUE(queue_push(&queue, &first));
    ASSERT_TRUE(queue_push(&queue, &second));
    ASSERT_EQ(queue_pop(&queue), &first);
    ASSERT_EQ(queue_pop(&queue), &second);
    queue_destroy(&queue);
}

TEST(QueueTest, PopReturnsNullAfterClose)
{
    request_queue queue{};

    ASSERT_TRUE(queue_init(&queue, 2));
    queue_close(&queue);
    ASSERT_EQ(queue_pop(&queue), nullptr);
    queue_destroy(&queue);
}

TEST(QueueTest, PopsRequestsBeforeReturningNullAfterClose)
{
    request_queue queue{};
    request request{1, 10};

    ASSERT_TRUE(queue_init(&queue, 2));
    ASSERT_TRUE(queue_push(&queue, &request));
    queue_close(&queue);
    ASSERT_EQ(queue_pop(&queue), &request);
    ASSERT_EQ(queue_pop(&queue), nullptr);
    queue_destroy(&queue);
}

TEST(QueueTest, RejectsPushAfterClose)
{
    request_queue queue{};
    request request{1, 10};

    ASSERT_TRUE(queue_init(&queue, 2));
    queue_close(&queue);
    ASSERT_FALSE(queue_push(&queue, &request));
    queue_destroy(&queue);
}

TEST(QueueTest, PushWaitsForAvailableCapacity)
{
    request_queue queue{};
    request first{1, 10};
    request second{2, 20};
    QueueThreadData data{&queue, &second, nullptr};
    pthread_t thread;

    ASSERT_TRUE(queue_init(&queue, 1));
    ASSERT_TRUE(queue_push(&queue, &first));
    ASSERT_EQ(pthread_create(&thread, nullptr, push_request, &data), 0);
    usleep(10000);
    ASSERT_EQ(queue_pop(&queue), &first);
    ASSERT_EQ(pthread_join(thread, nullptr), 0);
    ASSERT_EQ(data.result, &second);
    ASSERT_EQ(queue_pop(&queue), &second);
    queue_destroy(&queue);
}

TEST(QueueTest, PopWaitsForAvailableRequest)
{
    request_queue queue{};
    request queued{1, 10};
    QueueThreadData data{&queue, nullptr, nullptr};
    pthread_t thread;

    ASSERT_TRUE(queue_init(&queue, 1));
    ASSERT_EQ(pthread_create(&thread, nullptr, pop_request, &data), 0);
    usleep(10000);
    ASSERT_TRUE(queue_push(&queue, &queued));
    ASSERT_EQ(pthread_join(thread, nullptr), 0);
    ASSERT_EQ(data.result, &queued);
    queue_close(&queue);
    queue_destroy(&queue);
}

TEST(QueueTest, DestroyReleasesQueueMemory)
{
    request_queue queue{};
    size_t before = memory_outstanding();

    ASSERT_TRUE(queue_init(&queue, 2));
    queue_destroy(&queue);
    ASSERT_EQ(memory_outstanding(), before);
}

TEST(BookingTest, InitializesBooking)
{
    app application{};
    application.config.seats = 2;
    application.config.workers = 1;

    ASSERT_TRUE(booking_init(&application));
    ASSERT_EQ(application.train.seat_count, 2);
    ASSERT_EQ(application.train.available, 2);
    ASSERT_EQ(application.reservations.count, 0);
    booking_cleanup(&application);
}

TEST(BookingTest, BooksRequest)
{
    app application{};
    application.config.seats = 2;
    application.config.workers = 1;
    pthread_mutex_init(&application.output_mutex, nullptr);
    application.events = tmpfile();
    application.reservations_file = tmpfile();
    application.oom_log = tmpfile();
    request booking_request{1, 10};

    ASSERT_TRUE(booking_init(&application));
    ASSERT_TRUE(booking_process(&application, &booking_request));
    ASSERT_EQ(application.train.available, 1);
    ASSERT_EQ(application.reservations.count, 1);
    ASSERT_EQ(application.metrics.successful, 1);

    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);
    booking_cleanup(&application);
}

TEST(BookingTest, FailsWhenNoSeatsAreAvailable)
{
    app application{};
    application.config.seats = 1;
    application.config.workers = 1;
    pthread_mutex_init(&application.output_mutex, nullptr);
    application.events = tmpfile();
    application.reservations_file = tmpfile();
    application.oom_log = tmpfile();
    request first_request{1, 10};
    request second_request{2, 20};

    ASSERT_TRUE(booking_init(&application));
    ASSERT_TRUE(booking_process(&application, &first_request));
    ASSERT_FALSE(booking_process(&application, &second_request));
    ASSERT_EQ(application.train.available, 0);
    ASSERT_EQ(application.reservations.count, 1);
    ASSERT_EQ(application.metrics.failed, 1);

    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);
    booking_cleanup(&application);
}

TEST(BookingTest, RollsBackSeatWhenReservationAllocationFails)
{
    app application{};
    application.config.seats = 1;
    application.config.workers = 1;
    pthread_mutex_init(&application.output_mutex, nullptr);
    application.events = tmpfile();
    application.reservations_file = tmpfile();
    application.oom_log = tmpfile();
    request booking_request{1, 10};

    ASSERT_TRUE(booking_init(&application));
    memory_fail_after(0);
    ASSERT_FALSE(booking_process(&application, &booking_request));
    memory_fail_after(-1);
    ASSERT_EQ(application.train.available, 1);
    ASSERT_EQ(application.reservations.count, 0);
    ASSERT_EQ(application.metrics.reservation_oom_failures, 1);
    ASSERT_EQ(application.metrics.seat_rollbacks, 1);

    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);
    booking_cleanup(&application);
}

TEST(BookingTest, WorkerProcessesRequest)
{
    app application{};
    application.config.seats = 1;
    application.config.workers = 1;
    pthread_mutex_init(&application.output_mutex, nullptr);
    application.events = tmpfile();
    application.reservations_file = tmpfile();
    application.oom_log = tmpfile();
    request* booking_request = static_cast<request*>(memory_alloc(sizeof(request)));
    pthread_t worker;

    ASSERT_TRUE(booking_init(&application));
    ASSERT_NE(booking_request, nullptr);
    booking_request->id = 1;
    booking_request->passenger_id = 10;
    ASSERT_TRUE(queue_push(&application.queue, booking_request));
    queue_close(&application.queue);
    ASSERT_EQ(pthread_create(&worker, nullptr, booking_worker, &application), 0);
    ASSERT_EQ(pthread_join(worker, nullptr), 0);
    ASSERT_EQ(application.metrics.successful, 1);
    ASSERT_EQ(application.reservations.count, 1);

    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);
    booking_cleanup(&application);
}

TEST(OutputTest, WritesEvent)
{
    app application{};
    pthread_mutex_init(&application.output_mutex, nullptr);
    application.events = tmpfile();

    output_event(&application, "test event");
    ASSERT_EQ(fflush(application.events), 0);
    ASSERT_EQ(fseek(application.events, 0, SEEK_SET), 0);

    char line[20] = {};
    ASSERT_NE(fgets(line, sizeof(line), application.events), nullptr);
    ASSERT_STREQ(line, "test event\n");

    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);
}

TEST(OutputTest, WritesOutOfMemoryEvent)
{
    app application{};
    pthread_mutex_init(&application.output_mutex, nullptr);
    application.oom_log = tmpfile();

    output_oom(&application, "out of memory");
    ASSERT_EQ(fflush(application.oom_log), 0);
    ASSERT_EQ(fseek(application.oom_log, 0, SEEK_SET), 0);

    char line[20] = {};
    ASSERT_NE(fgets(line, sizeof(line), application.oom_log), nullptr);
    ASSERT_STREQ(line, "out of memory\n");

    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);
}

TEST(OutputTest, WritesReservation)
{
    app application{};
    reservation booking_reservation{1, 2, 3, 4, nullptr};
    pthread_mutex_init(&application.output_mutex, nullptr);
    application.reservations_file = tmpfile();

    output_reservation(&application, &booking_reservation);
    ASSERT_EQ(fflush(application.reservations_file), 0);
    ASSERT_EQ(fseek(application.reservations_file, 0, SEEK_SET), 0);

    char line[50] = {};
    ASSERT_NE(fgets(line, sizeof(line), application.reservations_file), nullptr);
    ASSERT_STREQ(line, "1,2,3,4,CONFIRMED\n");

    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);
}

TEST(OutputTest, PreparesOutputFiles)
{
    char directory[] = "/tmp/tatkal-output-XXXXXX";
    app application{};
    application.config.output_dir = mkdtemp(directory);

    ASSERT_NE(application.config.output_dir, nullptr);
    ASSERT_TRUE(output_prepare(&application));
    ASSERT_NE(application.events, nullptr);
    ASSERT_NE(application.reservations_file, nullptr);
    ASSERT_NE(application.memory_log, nullptr);
    ASSERT_NE(application.oom_log, nullptr);
    ASSERT_NE(application.system_metrics_file, nullptr);

    pthread_mutex_init(&application.output_mutex, nullptr);
    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);
}

TEST(OutputTest, SamplesSystemMetrics)
{
    char directory[] = "/tmp/tatkal-system-metrics-XXXXXX";
    app application{};
    application.config.output_dir = mkdtemp(directory);

    ASSERT_NE(application.config.output_dir, nullptr);
    ASSERT_TRUE(output_prepare(&application));
    pthread_mutex_init(&application.output_mutex, nullptr);
    ASSERT_TRUE(output_start_system_metrics(&application));
    usleep(600000);
    output_stop_system_metrics(&application);

    char system_path[640];
    snprintf(system_path, sizeof(system_path), "%s/metrics/system.csv", application.run_dir);
    FILE *system_file = fopen(system_path, "r");
    ASSERT_NE(system_file, nullptr);
    char header[100] = {};
    char sample[100] = {};
    ASSERT_NE(fgets(header, sizeof(header), system_file), nullptr);
    ASSERT_STREQ(header, "elapsed_ms,rss_kb,vmsize_kb,threads,tracked_bytes,peak_tracked_bytes\n");
    ASSERT_NE(fgets(sample, sizeof(sample), system_file), nullptr);
    fclose(system_file);

    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);
}

TEST(OutputTest, WritesReports)
{
    char directory[] = "/tmp/tatkal-report-XXXXXX";
    app application{};
    application.config.seats = 2;
    application.config.requests = 3;
    application.config.workers = 1;
    application.config.output_dir = mkdtemp(directory);

    ASSERT_NE(application.config.output_dir, nullptr);
    ASSERT_TRUE(output_prepare(&application));
    pthread_mutex_init(&application.output_mutex, nullptr);
    application.metrics.submitted = 3;
    application.metrics.completed = 3;
    application.metrics.successful = 2;
    application.metrics.failed = 1;
    application.train.available = 0;
    application.metrics.started.tv_sec = 1;
    application.metrics.started.tv_nsec = 0;

    output_write_reports(&application);
    output_close(&application);
    pthread_mutex_destroy(&application.output_mutex);

    char metrics_path[640];
    char summary_path[640];
    snprintf(metrics_path, sizeof(metrics_path), "%s/metrics/metrics.csv", application.run_dir);
    snprintf(summary_path, sizeof(summary_path), "%s/summary.txt", application.run_dir);
    FILE* metrics_file = fopen(metrics_path, "r");
    FILE* summary_file = fopen(summary_path, "r");
    ASSERT_NE(metrics_file, nullptr);
    ASSERT_NE(summary_file, nullptr);
    fclose(metrics_file);
    fclose(summary_file);
}

TEST(OutputTest, RejectsEmptyOutputDirectory)
{
    app application{};
    application.config.output_dir = "";

    ASSERT_FALSE(output_prepare(&application));
}
