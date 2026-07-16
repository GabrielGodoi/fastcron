<p align="center">
  <h1 align="center">⚡ FastCron</h1>
  <p align="center">
    <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License"></a>
    <a href="https://Godoi.github.io/fastcron"><img src="https://img.shields.io/badge/docs-Sphinx-brightgreen.svg" alt="Docs"></a>
  </p>
</p>

---

## Motivation

FastCron does not invent any new scheduling concepts. It applies standard Linux/POSIX-like cron techniques to embedded systems. It was created simply because there was no existing C library that did this properly for microcontrollers without relying on runtime string parsing or dynamic memory allocation.

To evaluate the time efficiently on constrained devices, the library relies on established mathematical methods, specifically Sakamoto's algorithm for calculating the day of the week and Julian Day math for epoch conversions. The schedule itself is managed as a 24-byte bitmask.

## Applications

- **Deep sleep scheduling**: Calculates the exact time until the next cron event, which can be passed directly to sleep functions like `esp_deep_sleep()` or `k_sleep()`.
- **Offline environments**: Schedules can be stored in EEPROM and evaluated locally against an RTC module (e.g., DS3231) without requiring external connectivity.

## Design

The library is designed to have no OS dependencies, no string parsing, and no dynamic memory allocation. The `FastCron_t` struct can be initialized in three ways:

- **Remotely**: Populated by a server and sent as a binary payload over a network.
- **At runtime**: Using the provided C setter functions on the device.
- **At compile time**: Using static initialization for fixed schedules.

This implementation keeps the core logic minimal, requiring around 200 lines of C code.

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  External: User Application / Static Initializer        │
│                         │                               │
│              ┌──────────▼──────────┐                    │
│              │   FastCron_t Mask   │                    │
│              │                     │                    │
│              │  minutes: uint64_t  │  60 bits           │
│              │  hours:   uint32_t  │  24 bits           │
│              │  dom:     uint32_t  │  31 bits           │
│              │  months:  uint16_t  │  12 bits           │
│              │  dow:     uint8_t   │   7 bits           │
│              └──────────┬──────────┘  = 24 bytes        │
│                         │                               │
│                   ┌─────▼──────┐                        │
│                   │  Resolver  │  O(1) bit-scan per     │
│                   │            │  field, Julian Day     │
│                   │            │  epoch math (pure)     │
│                   └─────┬──────┘                        │
│                         │                               │
│                 ┌───────▼────────┐                      │
│                 │  Sleep Helper  │                      │
│                 │  (s / ms / µs) │                      │
│                 └───────┬────────┘                      │
│                         │                               │
│                  esp_deep_sleep(µs)                     │
└─────────────────────────────────────────────────────────┘
```

---

## Quick Start

### 1. Add to your CMake project

```cmake
add_subdirectory(fastcron)
target_link_libraries(your_app PRIVATE fastcron)
```

### 2. Runtime API initialization (ESP32 deep-sleep example)

```c
#include "fastcron.h"
#include <sys/time.h>
#include "esp_sleep.h"

void app_main(void)
{
    // "*/15 8-18 * * 1-5" — every 15 min, Mon–Fri, 8 AM to 6 PM
    FastCron_t schedule = {0};
    
    fastcron_set_minute(&schedule, 0);
    fastcron_set_minute(&schedule, 15);
    fastcron_set_minute(&schedule, 30);
    fastcron_set_minute(&schedule, 45);
    
    for (int i = 8; i <= 18; i++)
    {
        fastcron_set_hour(&schedule, i);
    }
    
    fastcron_set_all_days_of_month(&schedule);
    fastcron_set_all_months(&schedule);
    
    fastcron_set_day_of_week(&schedule, FASTCRON_MONDAY);
    fastcron_set_day_of_week(&schedule, FASTCRON_TUESDAY);
    fastcron_set_day_of_week(&schedule, FASTCRON_WEDNESDAY);
    fastcron_set_day_of_week(&schedule, FASTCRON_THURSDAY);
    fastcron_set_day_of_week(&schedule, FASTCRON_FRIDAY);

    struct timeval now;
    gettimeofday(&now, NULL);

    uint64_t sleep_us = 0;
    fastcron_sleep(&schedule, now.tv_sec, (uint32_t)now.tv_usec, NULL, NULL, &sleep_us);

    printf("Deep sleep for %llu µs (%.1f min)\n",
           sleep_us, (double)sleep_us / 60000000.0);

    esp_deep_sleep(sleep_us);
}
```

### 3. Static initialization

For fixed schedules, compute the bitmask offline and initialize it statically:

```c
// "*/15 8-18 * * 1-5"
static const FastCron_t static_schedule =
{
    .minutes       = (1ULL <<  0) | (1ULL << 15) | (1ULL << 30) | (1ULL << 45),
    .hours         = 0x0007FF00U,  // bits 8..18
    .days_of_month = 0xFFFFFFFEU,  // all days (1-31)
    .months        = 0x1FFE,       // all months (1-12)
    .days_of_week  = 0x3E,         // Mon(1)–Fri(5)
};
```

### 4. Direct epoch query

```c
time_t next = fastcron_get_next_wakeup(&schedule, time(NULL));
```

### 5. Array Scheduler (Multiple Crons)

If you have a list of distinct crons and want to find which one(s) will trigger next, use the `fastcron_scheduler` function. It safely copies the matching crons by value into your output array.

```c
FastCron_t my_crons[3];
// ... populate your crons ...

time_t now = time(NULL);

// 1. Ask the engine how many crons will trigger exactly at the next wakeup
size_t count = fastcron_scheduler(my_crons, 3, now, NULL, 0);
if (count == 0)
{
    printf("No future schedules found.\n");
    return;
}

// 2. Allocate an array and fetch them
FastCron_t next_schedules[count]; // Use VLA (or malloc for C90 strictness)
size_t written = fastcron_scheduler(my_crons, 3, now, next_schedules, count);

if (written > count)
{
    printf("Warning: Buffer was too small and output was truncated!\n");
}

// 3. Calculate the sleep time until the next event
uint32_t sleep_sec = 0;
fastcron_sleep(&next_schedules[0], now, 0, &sleep_sec, NULL, NULL);
printf("Going to sleep for %u seconds.\n", sleep_sec);

// After waking up, you can iterate `next_schedules` to run the respective tasks
for (size_t i = 0; i < written; i++)
{
    // Dispatch routines bound to next_schedules[i]
}
```

---

## API Reference

| Function | Description |
|---|---|
| `fastcron_get_next_wakeup(mask, epoch)` | Next matching UTC epoch |
| `fastcron_sleep(mask, tv_sec, tv_usec, *s, *ms, *us)` | Unified sleep calc with optional outputs |
| `fastcron_scheduler(crons, size, epoch, out, out_size)` | Finds the next triggering crons from an array |

### `FastCron_t` Bitmask Fields

| Field | Type | Bits Used | Encoding |
|-------|------|-----------|----------|
| `minutes` | `uint64_t` | 0–59 | bit N = minute N |
| `hours` | `uint32_t` | 0–23 | bit N = hour N |
| `days_of_month` | `uint32_t` | 1–31 | bit N = day N (bit 0 unused) |
| `months` | `uint16_t` | 1–12 | bit N = month N (bit 0 unused) |
| `days_of_week` | `uint8_t` | 0–6 | bit 0 = Sunday, bit 6 = Saturday |

---

## Performance Comparison

| Metric | FastCron | ccronexpr |
|---|---|---|
| **Algorithm** | Bounded O(k) bitmask | O(N) array iteration |
| **RAM per schedule** | **24 bytes** (stack) | ~120+ bytes (parsed) |
| **Flash footprint (.text)** | **~1.2 KB** | ~7.4 KB |
| **malloc calls** | **0** (Static) | Multiple `malloc()`s |
| **Avg resolution time** | **102 ns** | ~91.5 µs (91,561 ns) |
| **OS dependencies** | **None** | libc (`mktime`, `malloc`) |
| **MISRA compliant** | Yes | No |
| **Deep-sleep ready** | Yes (µs precision) | No |

---

## MISRA C:2012 Compliance

- **No dynamic memory allocation** — `malloc`, `calloc`, `realloc`, `free` never called.
- **No recursion** — all functions use bounded iteration (hard cap: 800).
- **No floating-point arithmetic** in the engine.
- **No platform `#ifdef` chains** — same code path everywhere.
- **No string operations** — no `<string.h>` dependency.
- **Explicit casts** — no implicit type conversions.

---

## Building

```bash
cmake -B build -DFASTCRON_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## Project Structure

```
fastcron/
├── include/
│   └── fastcron.h          # Public API
├── src/
│   └── fastcron.c          # Pure-math engine
├── tests/
│   └── test_fastcron.c     # Unity test suite
├── docs/
│   ├── Doxyfile            
│   ├── conf.py             
│   ├── index.rst           
│   └── api.rst             
├── .github/
│   └── workflows/
│       └── main.yml        
├── CMakeLists.txt
├── LICENSE                 
└── README.md
```

---

## License

[MIT](LICENSE)
