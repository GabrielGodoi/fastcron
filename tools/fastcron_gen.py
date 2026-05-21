#!/usr/bin/env python3
"""
fastcron_gen.py — Cron string → FastCron_t C bitmask generator.

Uses the industry-standard `croniter` library for parsing.
Outputs ready-to-paste C struct initializers or JSON.

Usage:
    python fastcron_gen.py "*/15 8-18 * * 1-5"
    python fastcron_gen.py --json "0 9 1,15 * 1-5"
    python fastcron_gen.py --header "*/5 * * * *" "0 8 * * 1-5"
"""

import argparse
import json
import sys
from dataclasses import dataclass

from croniter import croniter

FIELD_RANGES = {
    "minutes":       (0, 59),
    "hours":         (0, 23),
    "days_of_month": (1, 31),
    "months":        (1, 12),
    "days_of_week":  (0, 6),
}


@dataclass(frozen=True)
class FastCronMask:
    minutes: int
    hours: int
    days_of_month: int
    months: int
    days_of_week: int


def expand_field(raw_values: list, field_min: int, field_max: int) -> int:
    if raw_values == ["*"]:
        bitmask = 0
        for i in range(field_min, field_max + 1):
            bitmask |= 1 << i
        return bitmask

    bitmask = 0
    for v in raw_values:
        bitmask |= 1 << int(v)
    return bitmask


def cron_to_mask(cron_str: str) -> FastCronMask:
    fields, _ = croniter.expand(cron_str)

    if len(fields) < 5:
        raise ValueError(f"Expected 5 fields, got {len(fields)}: '{cron_str}'")

    names = list(FIELD_RANGES.keys())
    values = {}
    for i, name in enumerate(names):
        lo, hi = FIELD_RANGES[name]
        values[name] = expand_field(fields[i], lo, hi)

    return FastCronMask(**values)


def format_c_hex(mask: FastCronMask, var_name: str = "schedule") -> str:
    lines = [
        f"static const FastCron_t {var_name} =",
        "{",
        f"    .minutes       = 0x{mask.minutes:016X}ULL,",
        f"    .hours         = 0x{mask.hours:08X}U,",
        f"    .days_of_month = 0x{mask.days_of_month:08X}U,",
        f"    .months        = 0x{mask.months:04X},",
        f"    .days_of_week  = 0x{mask.days_of_week:02X},",
        "};",
    ]
    return "\n".join(lines)


def format_json(mask: FastCronMask) -> str:
    return json.dumps(
        {
            "minutes":       mask.minutes,
            "hours":         mask.hours,
            "days_of_month": mask.days_of_month,
            "months":        mask.months,
            "days_of_week":  mask.days_of_week,
        },
        indent=2,
    )


def format_header(masks: list[tuple[str, FastCronMask]]) -> str:
    guard = "FASTCRON_SCHEDULES_H"
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        '#include "fastcron.h"',
        "",
    ]

    for i, (expr, mask) in enumerate(masks):
        safe_name = f"schedule_{i}"
        comment_safe = expr.replace("*/", "*\\/")
        lines.append(f"/* {comment_safe} */")
        lines.append(format_c_hex(mask, safe_name))
        lines.append("")

    lines.append(f"#endif /* {guard} */")
    return "\n".join(lines)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Convert cron expressions to FastCron_t C bitmasks.",
    )
    parser.add_argument(
        "expressions",
        nargs="+",
        help="One or more 5-field cron expressions (quoted).",
    )

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--json",
        action="store_true",
        help="Output JSON instead of C struct.",
    )
    group.add_argument(
        "--header",
        action="store_true",
        help="Output a complete .h file with all schedules.",
    )

    parser.add_argument(
        "--name",
        default="schedule",
        help="C variable name (default: schedule). Ignored with --header.",
    )
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    masks: list[tuple[str, FastCronMask]] = []
    for expr in args.expressions:
        try:
            mask = cron_to_mask(expr)
        except (ValueError, KeyError) as e:
            print(f"Error parsing '{expr}': {e}", file=sys.stderr)
            return 1

        masks.append((expr, mask))

    if args.header:
        print(format_header(masks))
        return 0

    for expr, mask in masks:
        if len(masks) > 1:
            comment_safe = expr.replace("*/", "*\\/")
            print(f"/* {comment_safe} */")

        if args.json:
            print(format_json(mask))
        else:
            name = args.name if len(masks) == 1 else f"{args.name}_{masks.index((expr, mask))}"
            print(format_c_hex(mask, name))

        if len(masks) > 1:
            print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
