#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fastcron.h"

void app_main(void)
{
    printf("\n");
    printf("==========================================\n");
    printf(" Hello World do QEMU ESP32!\n");
    printf("==========================================\n");
    printf("\n");

    printf("POC SUCCESS\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
}
