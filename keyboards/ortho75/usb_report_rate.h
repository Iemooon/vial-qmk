/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Report-rate limiter for the AT32F405 OTGHS port — same divider semantics
 * as Keychron's usb_report_rate (div 0 = every frame = 8K at HS, div 3 = 1K),
 * minus their VIA-launcher plumbing and RGB indicator, which this tree does
 * not carry.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(USB_REPORT_INTERVAL_ENABLE)

void    report_rate_init(void);
uint8_t report_rate_get_div(void);
bool    report_rate_set_div(uint8_t div);

#endif
