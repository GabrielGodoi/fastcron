#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fastcron.h"
#include "unity.h"

// Basic chore test just to validate the environment
void test_fastcron_chore(void)
{
    FastCron_t mask = {0};
    fastcron_set_minute(&mask, 30);
    
    // Validate the bit was set correctly (1ULL << 30)
    TEST_ASSERT_EQUAL_UINT64(1073741824ULL, mask.minutes);
}

void app_main(void)
{
    printf("\n");
    printf("==========================================\n");
    printf(" Iniciando Testes Unitários no QEMU...\n");
    printf("==========================================\n");
    printf("\n");

    UNITY_BEGIN();
    RUN_TEST(test_fastcron_chore);
    UNITY_END();

    // Give QEMU time to flush serial and be killed by timeout
    vTaskDelay(pdMS_TO_TICKS(1000));
}
