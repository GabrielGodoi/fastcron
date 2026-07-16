#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "fastcron.h"
#include "ccronexpr.h"

int main(void)
{
    printf("========================================================\n");
    printf("API Usage Comparison: ccronexpr vs FastCron\n");
    printf("Target Schedule: 14:30 on the 15th of June\n");
    printf("========================================================\n\n");

    time_t currentEpoch = 1704067200LL; // 2024-01-01 00:00:00 UTC

    // ---------------------------------------------------------
    // 1. ccronexpr (Standard String-based API)
    // ---------------------------------------------------------
    printf("--- [ ccronexpr (String Parser) ] ---\n");
    
    cron_expr expr;
    const char* error = NULL;
    
    // Parse the string (requires malloc internally, prone to string typos)
    cron_parse_expr("0 30 14 15 6 *", &expr, &error);
    if (error != NULL)
    {
        return 1;
    }

    time_t ccronTime = currentEpoch;
    for (int i = 0; i < 3; i++)
    {
        ccronTime = cron_next(&expr, ccronTime);
        if (ccronTime == (time_t)-1)
        {
            return 1;
        }

        struct tm* timeInfo = gmtime(&ccronTime);
        if (timeInfo == NULL) return 1;

        uint32_t sleepSeconds = (uint32_t)(ccronTime - currentEpoch); // Manual diff required

        char buffer[64];
        strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", timeInfo);
        printf("ccronexpr trigger %d: %s (Sleep time: %u s)\n", i + 1, buffer, sleepSeconds);
        
        currentEpoch = ccronTime;
    }
    printf("\n");

    // ---------------------------------------------------------
    // 2. FastCron (Zero-Allocation Bitmask API)
    // ---------------------------------------------------------
    printf("--- [ FastCron (Bitmask Helpers) ] ---\n");

    FastCron_t schedule = {0};

    // Set exactly the values via type-safe macros (no malloc, compiler checked)
    fastcron_set_minute(&schedule, 30);
    fastcron_set_hour(&schedule, 14);
    fastcron_set_day_of_month(&schedule, 15);
    fastcron_set_month(&schedule, 6);
    fastcron_set_all_days_of_week(&schedule);

    time_t fastcronTime = 1704067200LL; // Reset epoch
    for (int i = 0; i < 3; i++)
    {
        uint32_t sleepSeconds = 0;
        fastcron_sleep(&schedule, fastcronTime, 0, &sleepSeconds, NULL, NULL); // Built-in API
        
        fastcronTime = fastcron_get_next_wakeup(&schedule, fastcronTime);
        if (fastcronTime == (time_t)-1)
        {
            return 1;
        }

        struct tm* timeInfo = gmtime(&fastcronTime);
        if (timeInfo == NULL) return 1;

        char buffer[64];
        strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", timeInfo);
        printf("FastCron  trigger %d: %s (Sleep time: %u s)\n", i + 1, buffer, sleepSeconds);
    }

    printf("\n");
    return 0;
}
