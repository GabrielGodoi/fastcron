#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fastcron.h"
#include "unity.h"

// Global Unity setUp and tearDown for the ESP32 test runner
void setUp(void) {}
void tearDown(void) {}

// Forward declare the runners exposed by the native tests
void run_test_fastcron(void);
void run_test_fastcron_scheduler(void);

void app_main(void)
{
    printf("\n");
    printf("==========================================\n");
    printf(" Iniciando Testes Nativos no QEMU ESP32...\n");
    printf("==========================================\n");
    printf("\n");

    UNITY_BEGIN();
    
    printf("--- Running test_fastcron.c ---\n");
    run_test_fastcron();
    
    printf("\n--- Running test_fastcron_scheduler.c ---\n");
    run_test_fastcron_scheduler();
    
    UNITY_END();

    // Give QEMU time to flush serial and be killed by timeout
    vTaskDelay(pdMS_TO_TICKS(1000));
}
