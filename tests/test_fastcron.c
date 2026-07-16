/**
 * @file test_fastcron.c
 * @brief Unit tests for FastCron using Unity — no string parser, pure bitmask init.
 */

#include "unity.h"
#include "fastcron.h"

void setUp(void)   {}
void tearDown(void) {}

/* -----------------------------------------------------------------------
 * Helper: pure-math epoch builder (mirrors the internal tm_to_epoch)
 * ----------------------------------------------------------------------- */

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

static void epoch_to_cal(time_t epoch, int *year, int *month, int *day,
                          int *hour, int *minute)
{
    int64_t s   = (int64_t)epoch;
    int64_t d   = s / 86400;
    int     rem = (int)(s % 86400);

    if (rem < 0)
    {
        d   -= 1;
        rem += 86400;
    }

    *hour   = rem / 3600;
    *minute = (rem % 3600) / 60;

    int64_t z   = d + 719468;
    int     era = (int)((z >= 0 ? z : z - 146096) / 146097);
    int     doe = (int)(z - (int64_t)era * 146097);
    int     yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int     yy  = yoe + era * 400;
    int     doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int     mp  = (5 * doy + 2) / 153;
    int     dd  = doy - (153 * mp + 2) / 5 + 1;
    int     mm  = mp + (mp < 10 ? 3 : -9);

    if (mm <= 2)
    {
        yy += 1;
    }

    *year  = yy;
    *month = mm;
    *day   = dd;
}

/* -----------------------------------------------------------------------
 * Bitmask factory helpers
 *
 * "every minute, every hour, every day, every month, every DOW"
 * ----------------------------------------------------------------------- */

#define ALL_MINUTES   (0x0FFFFFFFFFFFFFFFULL)
#define ALL_HOURS     (0x00FFFFFFU)
#define ALL_DOM       (0xFFFFFFFEU)
#define ALL_MONTHS    (0x1FFEU)
#define ALL_DOW       (0x7FU)

#define BIT64(n)  ((uint64_t)1U << (n))
#define BIT32(n)  ((uint32_t)1U << (n))
#define BIT16(n)  ((uint16_t)1U << (n))
#define BIT8(n)   ((uint8_t)1U  << (n))

static FastCron_t every_minute(void)
{
    FastCron_t m = { ALL_MINUTES, ALL_HOURS, ALL_DOM, ALL_MONTHS, ALL_DOW };
    return m;
}

/* -----------------------------------------------------------------------
 * Null safety
 * ----------------------------------------------------------------------- */

void test_null_mask_returns_error(void)
{
    TEST_ASSERT_EQUAL_INT64((time_t)-1, fastcron_get_next_wakeup(NULL, 0));
    TEST_ASSERT_FALSE(fastcron_sleep(NULL, 0, 0, NULL, NULL, NULL));
}

/* -----------------------------------------------------------------------
 * Every-minute schedule: next event <= 120 s ahead, on a minute boundary
 * ----------------------------------------------------------------------- */

void test_every_minute_next_is_close(void)
{
    FastCron_t mask = every_minute();

    time_t now  = make_epoch(2024, 6, 15, 10, 30);
    time_t next = fastcron_get_next_wakeup(&mask, now);

    TEST_ASSERT_TRUE(next > now);
    TEST_ASSERT_TRUE((next - now) <= 120);
    TEST_ASSERT_EQUAL_INT64(0, next % 60);
}

/* -----------------------------------------------------------------------
 * Specific hour:minute — "30 14 * * *"
 * ----------------------------------------------------------------------- */

void test_specific_hour_minute(void)
{
    FastCron_t mask =
    {
        .minutes       = BIT64(30),
        .hours         = BIT32(14),
        .days_of_month = ALL_DOM,
        .months        = ALL_MONTHS,
        .days_of_week  = ALL_DOW,
    };

    time_t now  = make_epoch(2024, 1, 1, 10, 0);
    time_t next = fastcron_get_next_wakeup(&mask, now);

    int y, mo, d, h, mn;
    epoch_to_cal(next, &y, &mo, &d, &h, &mn);

    TEST_ASSERT_EQUAL_INT(14, h);
    TEST_ASSERT_EQUAL_INT(30, mn);
    TEST_ASSERT_EQUAL_INT(2024, y);
    TEST_ASSERT_EQUAL_INT(1, mo);
    TEST_ASSERT_EQUAL_INT(1, d);
}

/* -----------------------------------------------------------------------
 * Same hour:minute but NOW is past that time => rolls to next day
 * ----------------------------------------------------------------------- */

void test_past_time_rolls_to_next_day(void)
{
    FastCron_t mask =
    {
        .minutes       = BIT64(0),
        .hours         = BIT32(8),
        .days_of_month = ALL_DOM,
        .months        = ALL_MONTHS,
        .days_of_week  = ALL_DOW,
    };

    time_t now  = make_epoch(2024, 3, 10, 9, 0);
    time_t next = fastcron_get_next_wakeup(&mask, now);

    int y, mo, d, h, mn;
    epoch_to_cal(next, &y, &mo, &d, &h, &mn);

    TEST_ASSERT_EQUAL_INT(8, h);
    TEST_ASSERT_EQUAL_INT(0, mn);
    TEST_ASSERT_EQUAL_INT(11, d);
}

/* -----------------------------------------------------------------------
 * Weekday constraint — Mon-Fri (bits 1..5), starting on Saturday
 * 2024-01-06 is a Saturday → must jump to Monday 2024-01-08
 * ----------------------------------------------------------------------- */

void test_weekday_constraint_skips_weekend(void)
{
    FastCron_t mask =
    {
        .minutes       = BIT64(0),
        .hours         = BIT32(9),
        .days_of_month = ALL_DOM,
        .months        = ALL_MONTHS,
        .days_of_week  = BIT8(1) | BIT8(2) | BIT8(3) | BIT8(4) | BIT8(5),
    };

    time_t sat  = make_epoch(2024, 1, 6, 12, 0);
    time_t next = fastcron_get_next_wakeup(&mask, sat);

    int y, mo, d, h, mn;
    epoch_to_cal(next, &y, &mo, &d, &h, &mn);

    TEST_ASSERT_EQUAL_INT(8, d);
    TEST_ASSERT_EQUAL_INT(9, h);
    TEST_ASSERT_EQUAL_INT(0, mn);
}

/* -----------------------------------------------------------------------
 * Month rollover — "0 0 1 3 *" from January → must reach March 1st
 * ----------------------------------------------------------------------- */

void test_month_rollover(void)
{
    FastCron_t mask =
    {
        .minutes       = BIT64(0),
        .hours         = BIT32(0),
        .days_of_month = BIT32(1),
        .months        = BIT16(3),
        .days_of_week  = ALL_DOW,
    };

    time_t jan  = make_epoch(2024, 1, 15, 0, 0);
    time_t next = fastcron_get_next_wakeup(&mask, jan);

    int y, mo, d, h, mn;
    epoch_to_cal(next, &y, &mo, &d, &h, &mn);

    TEST_ASSERT_EQUAL_INT(3, mo);
    TEST_ASSERT_EQUAL_INT(1, d);
    TEST_ASSERT_EQUAL_INT(0, h);
    TEST_ASSERT_EQUAL_INT(0, mn);
}

/* -----------------------------------------------------------------------
 * Year rollover — December schedule checked in late December
 * "0 0 25 12 *" checked on Dec 26 → must reach Dec 25 next year
 * ----------------------------------------------------------------------- */

void test_year_rollover(void)
{
    FastCron_t mask =
    {
        .minutes       = BIT64(0),
        .hours         = BIT32(0),
        .days_of_month = BIT32(25),
        .months        = BIT16(12),
        .days_of_week  = ALL_DOW,
    };

    time_t dec26 = make_epoch(2024, 12, 26, 0, 0);
    time_t next  = fastcron_get_next_wakeup(&mask, dec26);

    int y, mo, d, h, mn;
    epoch_to_cal(next, &y, &mo, &d, &h, &mn);

    TEST_ASSERT_EQUAL_INT(2025, y);
    TEST_ASSERT_EQUAL_INT(12, mo);
    TEST_ASSERT_EQUAL_INT(25, d);
}

/* -----------------------------------------------------------------------
 * Every-15-minutes bitmask: bits 0, 15, 30, 45
 * ----------------------------------------------------------------------- */

void test_every_15_minutes(void)
{
    FastCron_t mask =
    {
        .minutes       = BIT64(0) | BIT64(15) | BIT64(30) | BIT64(45),
        .hours         = ALL_HOURS,
        .days_of_month = ALL_DOM,
        .months        = ALL_MONTHS,
        .days_of_week  = ALL_DOW,
    };

    time_t now  = make_epoch(2024, 6, 1, 10, 16);
    time_t next = fastcron_get_next_wakeup(&mask, now);

    int y, mo, d, h, mn;
    epoch_to_cal(next, &y, &mo, &d, &h, &mn);

    TEST_ASSERT_EQUAL_INT(30, mn);
}

/* -----------------------------------------------------------------------
 * Leap year — Feb 29 must be reachable in 2024
 * ----------------------------------------------------------------------- */

void test_leap_year_feb29(void)
{
    FastCron_t mask =
    {
        .minutes       = BIT64(0),
        .hours         = BIT32(0),
        .days_of_month = BIT32(29),
        .months        = BIT16(2),
        .days_of_week  = ALL_DOW,
    };

    time_t jan  = make_epoch(2024, 1, 1, 0, 0);
    time_t next = fastcron_get_next_wakeup(&mask, jan);

    int y, mo, d, h, mn;
    epoch_to_cal(next, &y, &mo, &d, &h, &mn);

    TEST_ASSERT_EQUAL_INT(2024, y);
    TEST_ASSERT_EQUAL_INT(2, mo);
    TEST_ASSERT_EQUAL_INT(29, d);
}

/* -----------------------------------------------------------------------
 * sleep_s basic sanity
 * ----------------------------------------------------------------------- */

void test_sleep_s_positive(void)
{
    FastCron_t mask = every_minute();

    time_t now = make_epoch(2024, 6, 15, 10, 30);
    uint32_t s = 0;
    bool res = fastcron_sleep(&mask, now, 0, &s, NULL, NULL);

    TEST_ASSERT_TRUE(res);
    TEST_ASSERT_TRUE(s > 0);
    TEST_ASSERT_TRUE(s <= 120);
}

/* -----------------------------------------------------------------------
 * sleep_ms subtracts sub-second offset
 * ----------------------------------------------------------------------- */

void test_sleep_ms_sub_second_offset(void)
{
    FastCron_t mask = every_minute();

    time_t tv_sec   = make_epoch(2024, 6, 15, 10, 30);
    uint32_t tv_usec = 500000;

    uint64_t ms = 0;
    uint32_t s_raw = 0;
    
    bool res = fastcron_sleep(&mask, tv_sec, tv_usec, &s_raw, &ms, NULL);
    TEST_ASSERT_TRUE(res);

    uint64_t s_ms  = (uint64_t)s_raw * 1000U;

    TEST_ASSERT_TRUE(ms > 0);
    TEST_ASSERT_TRUE(ms < s_ms);

    uint64_t diff = s_ms - ms;
    TEST_ASSERT_TRUE(diff >= 400 && diff <= 600);
}

/* -----------------------------------------------------------------------
 * sleep_us is finer than sleep_ms
 * ----------------------------------------------------------------------- */

void test_sleep_us_is_finer(void)
{
    FastCron_t mask = every_minute();

    time_t tv_sec   = make_epoch(2024, 6, 15, 10, 30);
    uint32_t tv_usec = 123456;

    uint64_t us = 0;
    uint64_t ms = 0;
    
    bool res = fastcron_sleep(&mask, tv_sec, tv_usec, NULL, &ms, &us);
    TEST_ASSERT_TRUE(res);

    TEST_ASSERT_TRUE(us > 0);
    TEST_ASSERT_TRUE(us > ms);
}

/* -----------------------------------------------------------------------
 * Known epoch sanity — 2024-01-01 00:00 UTC = 1704067200
 * ----------------------------------------------------------------------- */

void test_epoch_sanity_check(void)
{
    time_t epoch = make_epoch(2024, 1, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT64(1704067200LL, epoch);
}

/* -----------------------------------------------------------------------
 * Runner
 * ----------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_null_mask_returns_error);
    RUN_TEST(test_every_minute_next_is_close);
    RUN_TEST(test_specific_hour_minute);
    RUN_TEST(test_past_time_rolls_to_next_day);
    RUN_TEST(test_weekday_constraint_skips_weekend);
    RUN_TEST(test_month_rollover);
    RUN_TEST(test_year_rollover);
    RUN_TEST(test_every_15_minutes);
    RUN_TEST(test_leap_year_feb29);
    RUN_TEST(test_sleep_s_positive);
    RUN_TEST(test_sleep_ms_sub_second_offset);
    RUN_TEST(test_sleep_us_is_finer);
    RUN_TEST(test_epoch_sanity_check);

    return UNITY_END();
}
