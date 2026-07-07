FastCron Documentation
======================

.. toctree::
   :maxdepth: 2
   :caption: Contents

   api

Overview
--------

**FastCron** is an ultra-fast, zero-allocation, bitmask-based cron scheduler
designed for battery-powered embedded systems.  It resolves the next cron event
in O(1) per field using hardware bit-scan intrinsics, making it ideal for
deep-sleep wake-up scheduling on MCUs like ESP32 and STM32.

Key Features
^^^^^^^^^^^^

- **Zero dynamic memory allocation** — stack-only, MISRA C:2012 compliant.
- **O(1) bitmask resolution** — uses ``__builtin_ctzll`` / ``_BitScanForward``.
- **Sub-second precision** — ``timeval``-aware sleep helpers for RTC hardware.
- **Portable** — GCC, Clang, MSVC, ESP-IDF, Zephyr, bare-metal.

Quick Start
^^^^^^^^^^^

.. code-block:: c

   #include "fastcron.h"

   FastCron_t schedule;
   fastcron_parse_string("*/15 8-18 * * 1-5", &schedule);

   struct timeval now;
   gettimeofday(&now, NULL);

   uint64_t sleep_us = fastcron_sleep_us(&schedule, &now);
   esp_deep_sleep(sleep_us);
