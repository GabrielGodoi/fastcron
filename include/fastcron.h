/**
 * @file fastcron.h
 * @brief Bitmask-based cron scheduler for embedded deep-sleep.
 * @author Gabriel Godoi
 * @version 1.0.0
 *
 * Pure math, no OS dependencies, no string parsing, no dynamic memory.
 * The FastCron_t struct is populated externally (static init or transcompiler).
 *
 * @copyright Copyright (c) 2026 — MIT License
 */

#ifndef FASTCRON_H
#define FASTCRON_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Days of the week enumeration.
 */
typedef enum
{
    FASTCRON_SUNDAY    = 0,
    FASTCRON_MONDAY    = 1,
    FASTCRON_TUESDAY   = 2,
    FASTCRON_WEDNESDAY = 3,
    FASTCRON_THURSDAY  = 4,
    FASTCRON_FRIDAY    = 5,
    FASTCRON_SATURDAY  = 6
} FastCronDayOfWeek_t;

/**
 * @brief Bitmask-encoded cron schedule.
 *
 * Each field stores one bit per valid unit value:
 * - @c minutes       — bits 0..59  (60 bits of a uint64_t)
 * - @c hours         — bits 0..23  (24 bits of a uint32_t)
 * - @c days_of_month — bits 1..31  (bit 0 unused)
 * - @c months        — bits 1..12  (bit 0 unused)
 * - @c days_of_week  — bits 0..6   (Sunday = 0)
 */
typedef struct
{
    uint64_t minutes;
    uint32_t hours;
    uint32_t days_of_month;
    uint16_t months;
    uint8_t  days_of_week;
} FastCron_t;

/* -----------------------------------------------------------------------
 * Bitmask Helper Functions
 * ----------------------------------------------------------------------- */

/**
 * @brief Sets a specific minute in the bitmask.
 * @param mask Pointer to the schedule.
 * @param min Minute to set (0-59).
 */
static inline void fastcron_set_minute(FastCron_t *mask, uint8_t min)
{
    if (mask == NULL || min > 59)
    {
        return;
    }
    mask->minutes |= (1ULL << min);
}

/**
 * @brief Clears a specific minute from the bitmask.
 * @param mask Pointer to the schedule.
 * @param min Minute to clear (0-59).
 */
static inline void fastcron_clear_minute(FastCron_t *mask, uint8_t min)
{
    if (mask == NULL || min > 59)
    {
        return;
    }
    mask->minutes &= ~(1ULL << min);
}

/**
 * @brief Sets all minutes in the bitmask (0-59).
 * @param mask Pointer to the schedule.
 */
static inline void fastcron_set_all_minutes(FastCron_t *mask)
{
    if (mask == NULL)
    {
        return;
    }
    mask->minutes = 0x0FFFFFFFFFFFFFFFULL;
}

/**
 * @brief Sets a specific hour in the bitmask.
 * @param mask Pointer to the schedule.
 * @param hour Hour to set (0-23).
 */
static inline void fastcron_set_hour(FastCron_t *mask, uint8_t hour)
{
    if (mask == NULL || hour > 23)
    {
        return;
    }
    mask->hours |= (1U << hour);
}

/**
 * @brief Clears a specific hour from the bitmask.
 * @param mask Pointer to the schedule.
 * @param hour Hour to clear (0-23).
 */
static inline void fastcron_clear_hour(FastCron_t *mask, uint8_t hour)
{
    if (mask == NULL || hour > 23)
    {
        return;
    }
    mask->hours &= ~(1U << hour);
}

/**
 * @brief Sets all hours in the bitmask (0-23).
 * @param mask Pointer to the schedule.
 */
static inline void fastcron_set_all_hours(FastCron_t *mask)
{
    if (mask == NULL)
    {
        return;
    }
    mask->hours = 0x00FFFFFFU;
}

/**
 * @brief Sets a specific day of the month in the bitmask.
 * @param mask Pointer to the schedule.
 * @param dom Day of the month to set (1-31).
 */
static inline void fastcron_set_day_of_month(FastCron_t *mask, uint8_t dom)
{
    if (mask == NULL || dom < 1 || dom > 31)
    {
        return;
    }
    mask->days_of_month |= (1U << dom);
}

/**
 * @brief Clears a specific day of the month from the bitmask.
 * @param mask Pointer to the schedule.
 * @param dom Day of the month to clear (1-31).
 */
static inline void fastcron_clear_day_of_month(FastCron_t *mask, uint8_t dom)
{
    if (mask == NULL || dom < 1 || dom > 31)
    {
        return;
    }
    mask->days_of_month &= ~(1U << dom);
}

/**
 * @brief Sets all days of the month in the bitmask (1-31).
 * @param mask Pointer to the schedule.
 */
static inline void fastcron_set_all_days_of_month(FastCron_t *mask)
{
    if (mask == NULL)
    {
        return;
    }
    mask->days_of_month = 0xFFFFFFFEU;
}

/**
 * @brief Sets a specific month in the bitmask.
 * @param mask Pointer to the schedule.
 * @param month Month to set (1-12).
 */
static inline void fastcron_set_month(FastCron_t *mask, uint8_t month)
{
    if (mask == NULL || month < 1 || month > 12)
    {
        return;
    }
    mask->months |= (1U << month);
}

/**
 * @brief Clears a specific month from the bitmask.
 * @param mask Pointer to the schedule.
 * @param month Month to clear (1-12).
 */
static inline void fastcron_clear_month(FastCron_t *mask, uint8_t month)
{
    if (mask == NULL || month < 1 || month > 12)
    {
        return;
    }
    mask->months &= ~(1U << month);
}

/**
 * @brief Sets all months in the bitmask (1-12).
 * @param mask Pointer to the schedule.
 */
static inline void fastcron_set_all_months(FastCron_t *mask)
{
    if (mask == NULL)
    {
        return;
    }
    mask->months = 0x1FFEU;
}

/**
 * @brief Sets a specific day of the week in the bitmask.
 * @param mask Pointer to the schedule.
 * @param dow Day of the week to set (0-6, Sunday=0).
 */
static inline void fastcron_set_day_of_week(FastCron_t *mask, FastCronDayOfWeek_t dow)
{
    if (mask == NULL || (uint8_t)dow > 6)
    {
        return;
    }
    mask->days_of_week |= (1U << (uint8_t)dow);
}

/**
 * @brief Clears a specific day of the week from the bitmask.
 * @param mask Pointer to the schedule.
 * @param dow Day of the week to clear (0-6, Sunday=0).
 */
static inline void fastcron_clear_day_of_week(FastCron_t *mask, FastCronDayOfWeek_t dow)
{
    if (mask == NULL || (uint8_t)dow > 6)
    {
        return;
    }
    mask->days_of_week &= ~(1U << (uint8_t)dow);
}

/**
 * @brief Sets all days of the week in the bitmask (0-6).
 * @param mask Pointer to the schedule.
 */
static inline void fastcron_set_all_days_of_week(FastCron_t *mask)
{
    if (mask == NULL)
    {
        return;
    }
    mask->days_of_week = 0x7FU;
}

/**
 * @brief Compute the next matching epoch from a bitmask schedule.
 *
 * Uses O(1) bit-scan intrinsics per field. Pure math epoch conversion
 * with no OS/timezone dependencies.
 *
 * @param[in] mask          Pre-populated bitmask schedule.
 * @param[in] current_epoch Current UNIX timestamp (UTC).
 * @return    Next matching epoch (always > current_epoch), or (time_t)-1 on error.
 */
time_t fastcron_get_next_wakeup(const FastCron_t *mask, time_t current_epoch);

/**
 * @brief Whole-second sleep duration until the next cron event.
 *
 * @param[in] mask          Bitmask schedule.
 * @param[in] current_epoch Current UNIX timestamp (UTC).
 * @return    Seconds to sleep, or 0 on error.
 */
uint32_t fastcron_sleep_s(const FastCron_t *mask, time_t current_epoch);

/**
 * @brief Millisecond-accurate sleep duration using RTC sub-second offset.
 *
 * @param[in] mask   Bitmask schedule.
 * @param[in] tv_sec Current whole seconds (UTC epoch).
 * @param[in] tv_usec Current microsecond fraction (0–999999).
 * @return    Milliseconds to sleep (uint64_t to prevent overflow), or 0 on error.
 */
uint64_t fastcron_sleep_ms(const FastCron_t *mask, time_t tv_sec, uint32_t tv_usec);

/**
 * @brief Microsecond-accurate sleep duration using RTC sub-second offset.
 *
 * @param[in] mask   Bitmask schedule.
 * @param[in] tv_sec Current whole seconds (UTC epoch).
 * @param[in] tv_usec Current microsecond fraction (0–999999).
 * @return    Microseconds to sleep (uint64_t), or 0 on error.
 */
uint64_t fastcron_sleep_us(const FastCron_t *mask, time_t tv_sec, uint32_t tv_usec);

/**
 * @brief Unified helper function to find the crons to be executed.
 *
 * Finds the nearest future wakeup time across all provided schedules.
 * If multiple schedules trigger at that exact time, they are all collected.
 * 
 * Usage pattern:
 * 1. Call with `schedules = NULL` to get the number of pending crons.
 * 2. Allocate an array of `const FastCron_t*` of that size.
 * 3. Call again passing the allocated array and its size to populate it.
 *
 * @param[in]  crons          Array of bitmask schedules.
 * @param[in]  crons_size     Number of schedules in the `crons` array.
 * @param[in]  current_epoch  Current UNIX timestamp (UTC).
 * @param[out] schedules      Pointer to an array of `const FastCron_t*`. Pass NULL to just get the count.
 * @param[in]  schedules_size Maximum number of pointers that `schedules` can hold.
 * @return     Total number of crons that will trigger simultaneously on the next wakeup.
 *             If return value > schedules_size, the array was truncated.
 */
size_t fastcron_scheduler(
    const FastCron_t *crons,
    size_t crons_size,
    time_t current_epoch,
    const FastCron_t **schedules,
    size_t schedules_size
);

#ifdef __cplusplus
}
#endif

#endif /* FASTCRON_H */
