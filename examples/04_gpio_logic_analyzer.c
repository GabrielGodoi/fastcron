#include <stdint.h>
#include "fastcron.h"

#define GPIO_PORT_B_BSRR (*(volatile uint32_t*)0x40020418)
#define PIN_5_HIGH (1U << 5)
#define PIN_5_LOW  (1U << 21)

void measureExecutionTime(void)
{
    FastCron_t schedule =
    {
        .minutes       = (1ULL << 0),
        .hours         = (1U << 12),
        .days_of_month = 0xFFFFFFFEU,
        .months        = 0x1FFE,
        .days_of_week  = 0x7FU,
    };

    fastcron_time_t currentEpoch = 1704067200LL;

    GPIO_PORT_B_BSRR = PIN_5_HIGH;
    
    volatile fastcron_time_t nextWakeup = fastcron_get_next_wakeup(&schedule, currentEpoch);

    GPIO_PORT_B_BSRR = PIN_5_LOW;
    
    if (nextWakeup == (fastcron_time_t)-1)
    {
        return;
    }
}

int main(void)
{
    measureExecutionTime();
    return 0;
}
