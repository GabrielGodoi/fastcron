#include <stdio.h>
#include <time.h>
#include "fastcron.h"

int main(void)
{
    FastCron_t schedule = {0};

    fastcron_set_minute(&schedule, 30);
    fastcron_set_hour(&schedule, 14);
    fastcron_set_all_days_of_month(&schedule);
    fastcron_set_all_months(&schedule);
    fastcron_set_day_of_week(&schedule, FASTCRON_MONDAY);
    fastcron_set_day_of_week(&schedule, FASTCRON_WEDNESDAY);
    fastcron_set_day_of_week(&schedule, FASTCRON_FRIDAY);

    printf("Schedule Configured: 14:30 on Mondays, Wednesdays, and Fridays.\n\n");

    time_t currentEpoch = time(NULL);

    for (int i = 0; i < 5; i++)
    {
        time_t nextWakeup = fastcron_get_next_wakeup(&schedule, currentEpoch);

        if (nextWakeup == (time_t)-1)
        {
            printf("Error calculating next wakeup.\n");
            return 1;
        }

        uint32_t sleepSeconds = fastcron_sleep_s(&schedule, currentEpoch);

        struct tm* timeInfo = gmtime(&nextWakeup);
        if (timeInfo == NULL)
        {
            return 1;
        }

        char buffer[64];
        strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S (%A)", timeInfo);

        printf("Next trigger %d: %s (Sleep time: %u seconds)\n", i + 1, buffer, sleepSeconds);

        currentEpoch = nextWakeup;
    }

    return 0;
}
