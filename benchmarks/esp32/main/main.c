#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern int run_benchmarks(void);

void app_main(void)
{
    printf("Starting FastCron ESP32 HIL Benchmarks...\n");
    
    // Run the benchmark suite
    run_benchmarks();
    
    // Halt and wait
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
