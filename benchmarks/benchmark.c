#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "fastcron.h"
#include "ccronexpr.h"

void compileAndMeasureFootprint(void)
{
    printf("========================================\n");
    printf("Compiling Object Files for Footprint Analysis...\n");
    
    int compileFastCron = system("gcc -c src/fastcron.c -Os -Iinclude -o fastcron.o");
    if (compileFastCron != 0)
    {
        printf("Failed to compile fastcron.o\n");
        return;
    }

    int compileCcronexpr = system("gcc -c benchmarks/third_party/ccronexpr/ccronexpr.c -Os -o ccronexpr.o");
    if (compileCcronexpr != 0)
    {
        printf("Failed to compile ccronexpr.o\n");
        return;
    }
    
    printf("========================================\n");
    printf("Footprint (Flash .text / RAM .data .bss)\n");
    printf("========================================\n");
    
    int executeSize = system("size fastcron.o ccronexpr.o");
    if (executeSize != 0)
    {
        printf("Failed to execute size command\n");
        return;
    }
    
    printf("\n");
}

uint64_t benchmarkFastCronResolution(fastcron_time_t baseEpoch, uint32_t iterations)
{
    FastCron_t schedule =
    {
        .minutes       = (1ULL << 30),
        .hours         = (1U << 14),
        .days_of_month = (1U << 15),
        .months        = (1U << 6),
        .days_of_week  = (1U << 3)
    };

    uint64_t totalElapsedNs = 0;
    volatile fastcron_time_t accumulator = 0;
    uint32_t chunkSize = iterations / 100;

    for (uint32_t p = 0; p <= 100; p++)
    {
        printf("\r[FastCron]  Running O(1) engine... %3d%%", p);
        fflush(stdout);

        if (p == 100)
        {
            break;
        }

        struct timespec chunkStart;
        if (clock_gettime(CLOCK_MONOTONIC, &chunkStart) != 0)
        {
            return 0;
        }

        for (uint32_t i = 0; i < chunkSize; i++)
        {
            accumulator += fastcron_get_next_wakeup(&schedule, baseEpoch + (p * chunkSize) + i);
        }

        struct timespec chunkEnd;
        if (clock_gettime(CLOCK_MONOTONIC, &chunkEnd) != 0)
        {
            return 0;
        }

        totalElapsedNs += (chunkEnd.tv_sec - chunkStart.tv_sec) * 1000000000ULL + (chunkEnd.tv_nsec - chunkStart.tv_nsec);
    }
    printf("\n");

    if (accumulator == 0)
    {
        return 0;
    }

    return totalElapsedNs / iterations;
}

uint64_t benchmarkCcronexprResolution(fastcron_time_t baseEpoch, uint32_t iterations)
{
    cron_expr expr;
    const char* error = NULL;
    
    cron_parse_expr("0 30 14 15 6 3", &expr, &error);
    if (error != NULL)
    {
        printf("ccronexpr parsing failed: %s\n", error);
        return 0;
    }

    uint64_t totalElapsedNs = 0;
    volatile fastcron_time_t accumulator = 0;
    uint32_t chunkSize = iterations / 100;

    for (uint32_t p = 0; p <= 100; p++)
    {
        printf("\r[ccronexpr] Running O(N) arrays... %3d%%", p);
        fflush(stdout);

        if (p == 100)
        {
            break;
        }

        struct timespec chunkStart;
        if (clock_gettime(CLOCK_MONOTONIC, &chunkStart) != 0)
        {
            return 0;
        }

        for (uint32_t i = 0; i < chunkSize; i++)
        {
            accumulator += cron_next(&expr, baseEpoch + (p * chunkSize) + i);
        }

        struct timespec chunkEnd;
        if (clock_gettime(CLOCK_MONOTONIC, &chunkEnd) != 0)
        {
            return 0;
        }

        totalElapsedNs += (chunkEnd.tv_sec - chunkStart.tv_sec) * 1000000000ULL + (chunkEnd.tv_nsec - chunkStart.tv_nsec);
    }
    printf("\n");

    if (accumulator == 0)
    {
        return 0;
    }

    return totalElapsedNs / iterations;
}

uint64_t benchmarkFastCronScheduler(fastcron_time_t baseEpoch, uint32_t iterations)
{
    #define SCHEDULER_CRONS_COUNT 100
    FastCron_t crons[SCHEDULER_CRONS_COUNT];
    for (int i = 0; i < SCHEDULER_CRONS_COUNT; i++) {
        crons[i].minutes       = (1ULL << (i % 60)); // espalhados nos minutos
        crons[i].hours         = (1U << 14);
        crons[i].days_of_month = (1U << 15);
        crons[i].months        = (1U << 6);
        crons[i].days_of_week  = (1U << 3);
    }

    uint64_t totalElapsedNs = 0;
    volatile size_t accumulator = 0;
    uint32_t chunkSize = iterations / 100;
    FastCron_t schedules_out[SCHEDULER_CRONS_COUNT];

    for (uint32_t p = 0; p <= 100; p++)
    {
        printf("\r[Scheduler] Running array iteration... %3d%%", p);
        fflush(stdout);

        if (p == 100) break;

        struct timespec chunkStart;
        if (clock_gettime(CLOCK_MONOTONIC, &chunkStart) != 0) return 0;

        for (uint32_t i = 0; i < chunkSize; i++)
        {
            accumulator += fastcron_scheduler(crons, SCHEDULER_CRONS_COUNT, baseEpoch + (p * chunkSize) + i, schedules_out, SCHEDULER_CRONS_COUNT);
        }

        struct timespec chunkEnd;
        if (clock_gettime(CLOCK_MONOTONIC, &chunkEnd) != 0) return 0;

        totalElapsedNs += (chunkEnd.tv_sec - chunkStart.tv_sec) * 1000000000ULL + (chunkEnd.tv_nsec - chunkStart.tv_nsec);
    }
    printf("\n");

    if (accumulator == 0 && totalElapsedNs == 0) return 0;

    return totalElapsedNs / iterations;
}

int main(void)
{
    compileAndMeasureFootprint();

    printf("========================================\n");
    printf("Head-to-Head Performance Benchmark (1,000,000 runs)\n");
    printf("========================================\n");

    fastcron_time_t baseEpoch = 1704067200LL;
    uint32_t iterations = 1000000;

    uint64_t fastCronNs = benchmarkFastCronResolution(baseEpoch, iterations);
    if (fastCronNs == 0)
    {
        return 1;
    }

    uint64_t ccronNs = benchmarkCcronexprResolution(baseEpoch, iterations);
    if (ccronNs == 0)
    {
        return 1;
    }

    uint64_t schedulerNs = benchmarkFastCronScheduler(baseEpoch, iterations);
    if (schedulerNs == 0)
    {
        return 1;
    }

    printf("FastCron  (O(1) bit-scan)  : %llu ns per iteration\n", (unsigned long long)fastCronNs);
    printf("ccronexpr (O(N) array loop): %llu ns per iteration\n", (unsigned long long)ccronNs);
    printf("FastCron Scheduler (10 crons): %llu ns per iteration\n", (unsigned long long)schedulerNs);

    return 0;
}
