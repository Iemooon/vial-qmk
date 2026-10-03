/* SPDX-License-Identifier: GPL-2.0-or-later */

#include QMK_KEYBOARD_H

#if defined(USB_REPORT_INTERVAL_ENABLE)
#    include "usb_report_rate.h"

void keyboard_post_init_kb(void) {
    report_rate_init();
    keyboard_post_init_user();
}
#endif
