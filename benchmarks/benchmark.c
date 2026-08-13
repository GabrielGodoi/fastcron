#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "fastcron.h"
#include "ccronexpr.h"

#ifdef ESP_PLATFORM
#include "xtensa/core-macros.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static inline uint64_t get_bench_time(void) {
    return (uint64_t)XTHAL_GET_CCOUNT();
}
#else
static inline uint64_t get_bench_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif

#ifndef ESP_PLATFORM
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
#endif

uint64_t benchmarkFastCronResolution(time_t baseEpoch, uint32_t iterations)
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
    volatile time_t accumulator = 0;
    uint32_t chunkSize = iterations >= 100 ? iterations / 100 : iterations;
    uint32_t max_p = iterations >= 100 ? 100 : 1;

    for (uint32_t p = 0; p <= max_p; p++)
    {
        printf("\r[FastCron]  Running O(1) engine... %3d%%", (int)(p * (100 / max_p)));
        fflush(stdout);

        if (p == max_p)
        {
            break;
        }

        uint64_t chunkStart = get_bench_time();
        for (uint32_t i = 0; i < chunkSize; i++)
        {
            accumulator += fastcron_get_next_wakeup(&schedule, baseEpoch + (p * chunkSize) + i);
        }
        uint64_t chunkEnd = get_bench_time();
        
        // Handle 32-bit overflow safely if it happened (only matters for ESP32 CCOUNT, but we casted to 64-bit so we just mask)
        uint64_t diff = (chunkEnd - chunkStart) & 0xFFFFFFFFULL;
        #ifndef ESP_PLATFORM
        diff = chunkEnd - chunkStart;
        #endif
        
        totalElapsedNs += diff;

#ifdef ESP_PLATFORM
        vTaskDelay(1);
#endif
    }
    printf("\n");

    if (accumulator == 0)
    {
        return 0;
    }

    return totalElapsedNs / iterations;
}

uint64_t benchmarkCcronexprResolution(time_t baseEpoch, uint32_t iterations)
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
    volatile time_t accumulator = 0;
    uint32_t chunkSize = iterations >= 100 ? iterations / 100 : iterations;
    uint32_t max_p = iterations >= 100 ? 100 : 1;

    for (uint32_t p = 0; p <= max_p; p++)
    {
        printf("\r[ccronexpr] Running O(N) arrays... %3d%%", (int)(p * (100 / max_p)));
        fflush(stdout);

        if (p == max_p)
        {
            break;
        }

        uint64_t chunkStart = get_bench_time();
        for (uint32_t i = 0; i < chunkSize; i++)
        {
            accumulator += cron_next(&expr, baseEpoch + (p * chunkSize) + i);
        }
        uint64_t chunkEnd = get_bench_time();
        
        uint64_t diff = (chunkEnd - chunkStart) & 0xFFFFFFFFULL;
        #ifndef ESP_PLATFORM
        diff = chunkEnd - chunkStart;
        #endif
        
        totalElapsedNs += diff;
    }
    printf("\n");

    if (accumulator == 0)
    {
        return 0;
    }

    return totalElapsedNs / iterations;
}

uint64_t benchmarkFastCronScheduler(time_t baseEpoch, uint32_t iterations)
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
    uint32_t chunkSize = iterations >= 100 ? iterations / 100 : iterations;
    uint32_t max_p = iterations >= 100 ? 100 : 1;
    FastCron_t schedules_out[SCHEDULER_CRONS_COUNT];

    for (uint32_t p = 0; p <= max_p; p++)
    {
        printf("\r[Scheduler] Running array iteration... %3d%%", (int)(p * (100 / max_p)));
        fflush(stdout);

        if (p == max_p) break;

        uint64_t chunkStart = get_bench_time();
        for (uint32_t i = 0; i < chunkSize; i++)
        {
            accumulator += fastcron_scheduler(crons, SCHEDULER_CRONS_COUNT, baseEpoch + (p * chunkSize) + i, schedules_out, SCHEDULER_CRONS_COUNT);
        }
        uint64_t chunkEnd = get_bench_time();

        uint64_t diff = (chunkEnd - chunkStart) & 0xFFFFFFFFULL;
        #ifndef ESP_PLATFORM
        diff = chunkEnd - chunkStart;
        #endif
        
        totalElapsedNs += diff;
    }
    printf("\n");

    if (accumulator == 0 && totalElapsedNs == 0) return 0;

    return totalElapsedNs / iterations;
}

#ifdef ESP_PLATFORM
int run_benchmarks(uint32_t iterations)
#else
int main(int argc, char *argv[])
#endif
{
#ifndef ESP_PLATFORM
    compileAndMeasureFootprint();
#else
    // Delay for ESP32 monitor to catch up
    vTaskDelay(pdMS_TO_TICKS(1000));
#endif

    time_t baseEpoch = 1704067200LL;
#ifndef ESP_PLATFORM
    uint32_t iterations = 1000000;
    if (argc > 1)
    {
        int parsed = atoi(argv[1]);
        if (parsed > 0)
        {
            iterations = (uint32_t)parsed;
        }
    }
#endif

    printf("========================================\n");
    printf("Head-to-Head Performance Benchmark (%lu runs)\n", (unsigned long)iterations);
    printf("========================================\n");

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

#ifdef ESP_PLATFORM
    const char* unit = "CPU cycles";
#else
    const char* unit = "ns";
#endif

    printf("FastCron  (O(1) bit-scan)  : %llu %s per iteration\n", (unsigned long long)fastCronNs, unit);
    printf("ccronexpr (O(N) array loop): %llu %s per iteration\n", (unsigned long long)ccronNs, unit);
    printf("FastCron Scheduler (10 crons): %llu %s per iteration\n", (unsigned long long)schedulerNs, unit);
    
    printf("\n--- END OF BENCHMARK ---\n");

    return 0;
}
