#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "fastcron.h"

void handleMqttPayload(const uint8_t* payload, size_t length)
{
    if (payload == NULL)
    {
        return;
    }

    if (length != sizeof(FastCron_t))
    {
        return;
    }

    FastCron_t schedule;
    memcpy(&schedule, payload, sizeof(FastCron_t));

    fastcron_time_t currentEpoch = 1704067200LL;
    fastcron_time_t nextWakeup = fastcron_get_next_wakeup(&schedule, currentEpoch);

    if (nextWakeup == (fastcron_time_t)-1)
    {
        return;
    }

    printf("Payload processed. Next wakeup: %lld\n", (long long)nextWakeup);
}

int main(void)
{
    uint8_t rawPayload[24] = {0};
    rawPayload[0] = 0x01;
    handleMqttPayload(rawPayload, sizeof(rawPayload));
    return 0;
}
