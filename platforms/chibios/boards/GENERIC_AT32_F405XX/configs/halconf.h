/* Copyright 2026 Lemon
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include_next <halconf.h>

/* External EEPROM on I2C1 (PB6 SCL / PB7 SDA) needs the HAL I2C driver; the
 * generic halconf leaves it off because a matrix-only keyboard has no bus. */
#undef HAL_USE_I2C
#define HAL_USE_I2C TRUE
