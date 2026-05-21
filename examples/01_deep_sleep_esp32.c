#include <stdio.h>
#include <sys/time.h>
#include "fastcron.h"

void esp_deep_sleep(uint64_t time_in_us);

int main(void)
{
    FastCron_t schedule =
    {
        .minutes       = (1ULL << 0) | (1ULL << 15) | (1ULL << 30) | (1ULL << 45),
        .hours         = 0x0007FF00U,
        .days_of_month = 0xFFFFFFFEU,
        .months        = 0x1FFE,
        .days_of_week  = 0x3E,
    };

    struct timeval currentTime;
    if (gettimeofday(&currentTime, NULL) != 0)
    {
        return 1;
    }

    uint64_t sleepDurationUs = fastcron_sleep_us(&schedule, currentTime.tv_sec, (uint32_t)currentTime.tv_usec);

    if (sleepDurationUs == 0)
    {
        return 1;
    }

    esp_deep_sleep(sleepDurationUs);

    return 0;
}

void esp_deep_sleep(uint64_t time_in_us)
{
    printf("Entering deep sleep for %llu us\n", (unsigned long long)time_in_us);
}
