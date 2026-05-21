#include <stdio.h>
#include <stdint.h>
#include "fastcron.h"

void toggleFriday(FastCron_t* schedule)
{
    if (schedule == NULL)
    {
        return;
    }

    if ((schedule->days_of_week & (1U << FASTCRON_FRIDAY)) != 0)
    {
        fastcron_clear_day_of_week(schedule, FASTCRON_FRIDAY);
    }
    else
    {
        fastcron_set_day_of_week(schedule, FASTCRON_FRIDAY);
    }
}

int main(void)
{
    FastCron_t schedule = {0};

    fastcron_set_minute(&schedule, 0);
    fastcron_set_hour(&schedule, 8);
    fastcron_set_all_days_of_month(&schedule);
    fastcron_set_all_months(&schedule);
    fastcron_set_day_of_week(&schedule, FASTCRON_MONDAY);
    fastcron_set_day_of_week(&schedule, FASTCRON_TUESDAY);
    fastcron_set_day_of_week(&schedule, FASTCRON_WEDNESDAY);
    fastcron_set_day_of_week(&schedule, FASTCRON_THURSDAY);

    printf("Before toggle: %02X\n", schedule.days_of_week);
    toggleFriday(&schedule);
    printf("After toggle: %02X\n", schedule.days_of_week);

    return 0;
}
