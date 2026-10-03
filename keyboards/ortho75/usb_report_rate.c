/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * AT32F405 8K report-rate storage and application.
 *
 * The divider meaning is Keychron's: interval = (1 << div) - 1 frames skipped
 * between reports, so div = 0 sends on every frame (8 kHz when the device
 * enumerates at high speed and bInterval is 1) and div = 3 gives 1 kHz.
 * div is persisted in the otherwise unused EECONFIG_USER dword so a fresh
 * or erased EEPROM (0xFF) lands on div = 0 = 8K.
 */

#if defined(USB_REPORT_INTERVAL_ENABLE)

#include "usb_report_rate.h"

#include "eeconfig.h"
#include "quantum.h"
#include "usb_main.h"

extern void update_usb_report_interval(USBDriver *usbp, uint8_t interval);

static uint8_t report_rate_div = 0;

void report_rate_update_interval(void) {
    update_usb_report_interval(&USB_DRIVER, (0x01U << report_rate_div) - 1);
}

bool report_rate_set_div(uint8_t div) {
    if (div > 6) return false;

    report_rate_div = div;
    eeconfig_update_user(div);
    report_rate_update_interval();

    return true;
}

uint8_t report_rate_get_div(void) {
    return report_rate_div;
}

void report_rate_init(void) {
    if (!eeconfig_is_enabled()) {
        eeconfig_init();
    }

    report_rate_div = (uint8_t)(eeconfig_read_user() & 0xFF);
    if (report_rate_div > 6) {
        report_rate_div = 0;
    }

    report_rate_update_interval();
}

#endif
