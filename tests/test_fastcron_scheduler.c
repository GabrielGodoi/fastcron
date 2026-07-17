#include "unity.h"
#include "fastcron.h"

#ifndef TEST_ASSERT_EQUAL_size_t
#define TEST_ASSERT_EQUAL_size_t(expected, actual) TEST_ASSERT_EQUAL_UINT32((uint32_t)(expected), (uint32_t)(actual))
#endif

#ifndef ESP_PLATFORM
void setUp(void) {}
void tearDown(void) {}
#endif

#define ALL_MINUTES   (0x0FFFFFFFFFFFFFFFULL)
#define ALL_HOURS     (0x00FFFFFFU)
#define ALL_DOM       (0xFFFFFFFEU)
#define ALL_MONTHS    (0x1FFEU)
#define ALL_DOW       (0x7FU)

#define BIT64(n)  ((uint64_t)1U << (n))
#define BIT32(n)  ((uint32_t)1U << (n))
#define BIT16(n)  ((uint16_t)1U << (n))
#define BIT8(n)   ((uint8_t)1U  << (n))

static time_t make_epoch(int year, int month, int day, int hour, int minute)
{
    int y = year;
    int m = month;

    if (m <= 2)
    {
        y -= 1;
        m += 12;
    }

    int days = (365 * y) + (y / 4) - (y / 100) + (y / 400);
    days += (306 * (m + 1)) / 10 + day - 428;
    days -= 719163;

    return (time_t)((days * 86400LL) + (hour * 3600LL) + (minute * 60LL));
}

void test_scheduler_multiple_identical_crons(void)
{
    FastCron_t crons[3];
    for (int i = 0; i < 3; i++) {
        // "0 12 * * *"
        crons[i].minutes       = BIT64(0);
        crons[i].hours         = BIT32(12);
        crons[i].days_of_month = ALL_DOM;
        crons[i].months        = ALL_MONTHS;
        crons[i].days_of_week  = ALL_DOW;
    }

    time_t now = make_epoch(2024, 6, 15, 10, 0); // 10:00

    size_t count = fastcron_scheduler(crons, 3, now, NULL, 0);
    TEST_ASSERT_EQUAL_size_t(3, count);

    FastCron_t schedules[3] = {0};
    count = fastcron_scheduler(crons, 3, now, schedules, 3);
    TEST_ASSERT_EQUAL_size_t(3, count);
    
    // TDD Green: Now it's a pass-by-value deep copy!
    // Address must differ (distinct memory blocks)
    TEST_ASSERT_TRUE(&crons[0] != &schedules[0]);
    TEST_ASSERT_TRUE(&crons[1] != &schedules[1]);
    TEST_ASSERT_TRUE(&crons[2] != &schedules[2]);
    // Values must be equal! (Whole struct comparison)
    TEST_ASSERT_EQUAL_MEMORY(&crons[0], &schedules[0], sizeof(FastCron_t));
    TEST_ASSERT_EQUAL_MEMORY(&crons[1], &schedules[1], sizeof(FastCron_t));
    TEST_ASSERT_EQUAL_MEMORY(&crons[2], &schedules[2], sizeof(FastCron_t));
}

void test_scheduler_leap_year_and_month_lengths(void)
{
    FastCron_t crons[4] = {0};
    
    // Feb 28 ("0 0 28 2 *")
    crons[0].minutes = BIT64(0); crons[0].hours = BIT32(0);
    crons[0].days_of_month = BIT32(28); crons[0].months = BIT16(2); crons[0].days_of_week = ALL_DOW;

    // Feb 29 (leap year only) ("0 0 29 2 *")
    crons[1].minutes = BIT64(0); crons[1].hours = BIT32(0);
    crons[1].days_of_month = BIT32(29); crons[1].months = BIT16(2); crons[1].days_of_week = ALL_DOW;

    // Apr 30 ("0 0 30 4 *")
    crons[2].minutes = BIT64(0); crons[2].hours = BIT32(0);
    crons[2].days_of_month = BIT32(30); crons[2].months = BIT16(4); crons[2].days_of_week = ALL_DOW;

    // May 31 ("0 0 31 5 *")
    crons[3].minutes = BIT64(0); crons[3].hours = BIT32(0);
    crons[3].days_of_month = BIT32(31); crons[3].months = BIT16(5); crons[3].days_of_week = ALL_DOW;

    time_t now = make_epoch(2024, 2, 27, 0, 0); // 2024 is a leap year

    FastCron_t scheds[4] = {0};
    
    // Passo 1: O relógio está em 27/Fev. O primeiro a disparar deve ser 28/Fev.
    size_t count = fastcron_scheduler(crons, 4, now, scheds, 4);
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_UINT32(crons[0].days_of_month, scheds[0].days_of_month); // 28/Fev
    
    // Passo 2: Avançamos o tempo para 28/Fev. Como 2024 é bissexto, o próximo é 29/Fev.
    now = make_epoch(2024, 2, 28, 0, 0);
    count = fastcron_scheduler(crons, 4, now, scheds, 4);
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_UINT32(crons[1].days_of_month, scheds[0].days_of_month); // 29/Fev
    
    // Passo 3: Vamos testar um ano NÃO bissexto (2023)
    now = make_epoch(2023, 2, 28, 0, 0);
    count = fastcron_scheduler(crons, 4, now, scheds, 4);
    TEST_ASSERT_EQUAL_size_t(1, count);
    // O cron de 29/Fev só vai disparar em 2024!
    // Logo, o cron de 30/Abril de 2023 vai disparar ANTES dele!
    TEST_ASSERT_EQUAL_UINT32(crons[2].days_of_month, scheds[0].days_of_month); // 30/Abril!
}

void test_scheduler_edge_case_mixed_invalid_crons(void)
{
    FastCron_t crons[2] = {0};
    
    // Feb 30 (invalid) ("0 0 30 2 *")
    crons[0].minutes = BIT64(0); crons[0].hours = BIT32(0);
    crons[0].days_of_month = BIT32(30); crons[0].months = BIT16(2); crons[0].days_of_week = ALL_DOW;

    // Mar 1 (valid) ("0 0 1 3 *")
    crons[1].minutes = BIT64(0); crons[1].hours = BIT32(0);
    crons[1].days_of_month = BIT32(1); crons[1].months = BIT16(3); crons[1].days_of_week = ALL_DOW;

    time_t now = make_epoch(2024, 2, 28, 0, 0);

    FastCron_t scheds[2] = {0};
    size_t count = fastcron_scheduler(crons, 2, now, scheds, 2);
    
    TEST_ASSERT_EQUAL_size_t(1, count);
    TEST_ASSERT_EQUAL_UINT32(crons[1].months, scheds[0].months); // Valid one is Mar 1
}

void test_scheduler_edge_case_exact_epoch_boundary(void)
{
    FastCron_t crons[1] = {0};
    
    // "0 12 * * *"
    crons[0].minutes = BIT64(0); crons[0].hours = BIT32(12);
    crons[0].days_of_month = ALL_DOM; crons[0].months = ALL_MONTHS; crons[0].days_of_week = ALL_DOW;

    time_t exact_now = make_epoch(2024, 6, 15, 12, 0); // Exactly on the minute!

    FastCron_t scheds[1] = {0};
    size_t count = fastcron_scheduler(crons, 1, exact_now, scheds, 1);
    
    TEST_ASSERT_EQUAL_size_t(1, count);
    
    // Verificamos se ele agendou para amanhã, e não para o exato segundo de hoje!
    time_t next_wakeup = fastcron_get_next_wakeup(&scheds[0], exact_now);
    TEST_ASSERT_TRUE(next_wakeup > exact_now);
    TEST_ASSERT_EQUAL_INT64((int64_t)exact_now + 86400, (int64_t)next_wakeup);
}

void test_scheduler_wrong_capacity_too_small(void)
{
    FastCron_t crons[3];
    for (int i = 0; i < 3; i++) {
        // "0 12 * * *"
        crons[i].minutes = BIT64(0); crons[i].hours = BIT32(12);
        crons[i].days_of_month = ALL_DOM; crons[i].months = ALL_MONTHS; crons[i].days_of_week = ALL_DOW;
    }

    time_t now = make_epoch(2024, 6, 15, 10, 0);

    FastCron_t schedules[3] = {0}; // Allocate 3
    size_t count = fastcron_scheduler(crons, 3, now, schedules, 2); // Pass capacity 2
    
    TEST_ASSERT_EQUAL_size_t(3, count); // Should return 3 matches even though capacity is 2
    
    // Assert if schedules contains the 2 correct crons (it truncated the 3rd)
    TEST_ASSERT_EQUAL_MEMORY(&crons[0], &schedules[0], sizeof(FastCron_t));
    TEST_ASSERT_EQUAL_MEMORY(&crons[1], &schedules[1], sizeof(FastCron_t));
    
    // Untouched memory remains 0!
    FastCron_t empty_cron = {0};
    TEST_ASSERT_EQUAL_MEMORY(&empty_cron, &schedules[2], sizeof(FastCron_t));
}

void test_scheduler_wrong_capacity_too_large(void)
{
    FastCron_t crons[2];
    for (int i = 0; i < 2; i++) {
        // "0 12 * * *"
        crons[i].minutes = BIT64(0); crons[i].hours = BIT32(12);
        crons[i].days_of_month = ALL_DOM; crons[i].months = ALL_MONTHS; crons[i].days_of_week = ALL_DOW;
    }

    time_t now = make_epoch(2024, 6, 15, 10, 0);

    FastCron_t schedules[5] = {0}; // Allocate 5!
    
    // The user passes capacity = 5, but there are only 2 matches.
    // The function must only write to schedules[0] and [1], leaving the rest intact!
    size_t count = fastcron_scheduler(crons, 2, now, schedules, 5); 
    
    TEST_ASSERT_EQUAL_size_t(2, count);
    
    // Assert matches were written
    TEST_ASSERT_EQUAL_MEMORY(&crons[0], &schedules[0], sizeof(FastCron_t));
    TEST_ASSERT_EQUAL_MEMORY(&crons[1], &schedules[1], sizeof(FastCron_t));
    
    // Assert the excess capacity was NOT touched by the function!
    FastCron_t empty_cron = {0};
    TEST_ASSERT_EQUAL_MEMORY(&empty_cron, &schedules[2], sizeof(FastCron_t));
    TEST_ASSERT_EQUAL_MEMORY(&empty_cron, &schedules[3], sizeof(FastCron_t));
    TEST_ASSERT_EQUAL_MEMORY(&empty_cron, &schedules[4], sizeof(FastCron_t));
}

void run_test_fastcron_scheduler(void)
{
    RUN_TEST(test_scheduler_multiple_identical_crons);
    RUN_TEST(test_scheduler_leap_year_and_month_lengths);
    RUN_TEST(test_scheduler_edge_case_mixed_invalid_crons);
    RUN_TEST(test_scheduler_edge_case_exact_epoch_boundary);
    RUN_TEST(test_scheduler_wrong_capacity_too_small);
    RUN_TEST(test_scheduler_wrong_capacity_too_large);
}

#ifndef ESP_PLATFORM
int main(void)
{
    UNITY_BEGIN();
    run_test_fastcron_scheduler();
    return UNITY_END();
}
#endif
