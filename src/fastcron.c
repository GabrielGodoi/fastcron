/**
 * @file fastcron.c
 * @brief FastCron core — pure-math bitmask cron engine, zero OS dependencies.
 */

#include "fastcron.h"

#define FASTCRON_ERROR_EPOCH   ((time_t)-1)
#define FASTCRON_MAX_ITERATIONS 800

/* -----------------------------------------------------------------------
 * Portable bit-scan intrinsics
 * ----------------------------------------------------------------------- */

static inline int ctz64(uint64_t v)
{
    int n = 0;
    if ((v & 0x00000000FFFFFFFFULL) == 0U) { n += 32; v >>= 32; }
    if ((v & 0x000000000000FFFFULL) == 0U) { n += 16; v >>= 16; }
    if ((v & 0x00000000000000FFULL) == 0U) { n +=  8; v >>=  8; }
    if ((v & 0x000000000000000FULL) == 0U) { n +=  4; v >>=  4; }
    if ((v & 0x0000000000000003ULL) == 0U) { n +=  2; v >>=  2; }
    if ((v & 0x0000000000000001ULL) == 0U) { n +=  1; }
    return n;
}

static inline int ctz32(uint32_t v)
{
    int n = 0;
    if ((v & 0x0000FFFFU) == 0U) { n += 16; v >>= 16; }
    if ((v & 0x000000FFU) == 0U) { n +=  8; v >>=  8; }
    if ((v & 0x0000000FU) == 0U) { n +=  4; v >>=  4; }
    if ((v & 0x00000003U) == 0U) { n +=  2; v >>=  2; }
    if ((v & 0x00000001U) == 0U) { n +=  1; }
    return n;
}

/* -----------------------------------------------------------------------
 * Pure-math UTC epoch ↔ calendar conversions (Julian Day based)
 * ----------------------------------------------------------------------- */

static time_t tm_to_epoch(int year, int month, int day, int hour, int minute)
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

static void epoch_to_fields(time_t epoch, int *year, int *month, int *day,
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

    int64_t z = d + 719468;
    int     era  = (int)((z >= 0 ? z : z - 146096) / 146097);
    int     doe  = (int)(z - (int64_t)era * 146097);
    int     yoe  = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int     y    = yoe + era * 400;
    int     doy  = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int     mp   = (5 * doy + 2) / 153;
    int     dd   = doy - (153 * mp + 2) / 5 + 1;
    int     mm   = mp + (mp < 10 ? 3 : -9);

    if (mm <= 2)
    {
        y += 1;
    }

    *year  = y;
    *month = mm;
    *day   = dd;
}

/* -----------------------------------------------------------------------
 * Pure-math day-of-week (Tomohiko Sakamoto)
 * ----------------------------------------------------------------------- */

static int day_of_week(int year, int month, int day)
{
    static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    int y = year;

    if (month < 3)
    {
        y -= 1;
    }

    return (y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7;
}

/* -----------------------------------------------------------------------
 * Pure-math days-in-month (handles leap years)
 * ----------------------------------------------------------------------- */

static int days_in_month(int year, int month)
{
    static const int table[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month != 2)
    {
        return table[month];
    }

    bool leap = ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
    return leap ? 29 : 28;
}

/* -----------------------------------------------------------------------
 * Bit-scan helpers: find lowest set bit at or above 'start'
 * ----------------------------------------------------------------------- */

static int find_next_bit64(uint64_t mask, int start)
{
    uint64_t shifted = mask >> (uint32_t)start;

    if (shifted == 0U)
    {
        return -1;
    }

    return start + ctz64(shifted);
}

static int find_next_bit32(uint32_t mask, int start)
{
    uint32_t shifted = mask >> (uint32_t)start;

    if (shifted == 0U)
    {
        return -1;
    }

    return start + ctz32(shifted);
}

/* -----------------------------------------------------------------------
 * fastcron_get_next_wakeup
 *
 * Starting one minute after current_epoch, walk each calendar field
 * using bit-scan intrinsics.  When a field overflows, reset it and
 * carry into the next-higher field.
 * ----------------------------------------------------------------------- */

time_t fastcron_get_next_wakeup(const FastCron_t *mask, time_t current_epoch)
{
    if (mask == NULL)
    {
        return FASTCRON_ERROR_EPOCH;
    }

    time_t base = current_epoch + 60;
    base -= (base % 60);

    int year, month, day, hour, minute;
    epoch_to_fields(base, &year, &month, &day, &hour, &minute);

    for (int guard = 0; guard < FASTCRON_MAX_ITERATIONS; guard++)
    {
        int m = find_next_bit32((uint32_t)mask->months, month);
        if (m < 0)
        {
            year++;
            month  = find_next_bit32((uint32_t)mask->months, 1);
            if (month < 0)
            {
                return FASTCRON_ERROR_EPOCH;
            }
            day    = 1;
            hour   = 0;
            minute = 0;
            continue;
        }

        if (m != month)
        {
            month  = m;
            day    = 1;
            hour   = 0;
            minute = 0;
        }

        int dim = days_in_month(year, month);
        if (day > dim)
        {
            month++;
            day    = 1;
            hour   = 0;
            minute = 0;
            continue;
        }

        int d = find_next_bit32(mask->days_of_month, day);
        if ((d < 0) || (d > dim))
        {
            month++;
            day    = 1;
            hour   = 0;
            minute = 0;
            continue;
        }

        if (d != day)
        {
            day    = d;
            hour   = 0;
            minute = 0;
        }

        int dow = day_of_week(year, month, day);
        if (((mask->days_of_week >> dow) & 1U) == 0U)
        {
            day++;
            hour   = 0;
            minute = 0;
            continue;
        }

        int h = find_next_bit32(mask->hours, hour);
        if (h < 0)
        {
            day++;
            hour   = 0;
            minute = 0;
            continue;
        }

        if (h != hour)
        {
            hour   = h;
            minute = 0;
        }

        int mn = find_next_bit64(mask->minutes, minute);
        if (mn < 0)
        {
            hour++;
            minute = 0;
            continue;
        }

        minute = mn;
        return tm_to_epoch(year, month, day, hour, minute);
    }

    return FASTCRON_ERROR_EPOCH;
}

/* -----------------------------------------------------------------------
 * Sleep helpers
 * ----------------------------------------------------------------------- */

uint32_t fastcron_sleep_s(const FastCron_t *mask, time_t current_epoch)
{
    if (mask == NULL)
    {
        return 0U;
    }

    time_t next = fastcron_get_next_wakeup(mask, current_epoch);
    if (next == FASTCRON_ERROR_EPOCH)
    {
        return 0U;
    }

    if (next <= current_epoch)
    {
        return 0U;
    }

    return (uint32_t)(next - current_epoch);
}

uint64_t fastcron_sleep_ms(const FastCron_t *mask, time_t tv_sec, uint32_t tv_usec)
{
    if (mask == NULL)
    {
        return 0U;
    }

    time_t next = fastcron_get_next_wakeup(mask, tv_sec);
    if (next == FASTCRON_ERROR_EPOCH)
    {
        return 0U;
    }

    if (next <= tv_sec)
    {
        return 0U;
    }

    uint64_t whole_ms = (uint64_t)(next - tv_sec) * 1000U;
    uint64_t sub_ms   = (uint64_t)tv_usec / 1000U;

    if (whole_ms <= sub_ms)
    {
        return 0U;
    }

    return whole_ms - sub_ms;
}

uint64_t fastcron_sleep_us(const FastCron_t *mask, time_t tv_sec, uint32_t tv_usec)
{
    if (mask == NULL)
    {
        return 0U;
    }

    time_t next = fastcron_get_next_wakeup(mask, tv_sec);
    if (next == FASTCRON_ERROR_EPOCH)
    {
        return 0U;
    }

    if (next <= tv_sec)
    {
        return 0U;
    }

    uint64_t whole_us = (uint64_t)(next - tv_sec) * 1000000U;
    uint64_t sub_us   = (uint64_t)tv_usec;

    if (whole_us <= sub_us)
    {
        return 0U;
    }

    return whole_us - sub_us;
}
