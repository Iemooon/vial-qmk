// Copyright 2023-2025 HorrorTroll <https://github.com/HorrorTroll>
// Copyright 2023-2025 Zhaqian <https://github.com/zhaqian12>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/*
 * The AT32F402 only has the full-speed OTG1 controller, so QMK keeps the
 * default USBD1 (see tmk_core/protocol/chibios/usb_main.h).
 */
#define BOARD_OTG_VBUSIG

#ifndef EARLY_INIT_PERFORM_BOOTLOADER_JUMP
#    define EARLY_INIT_PERFORM_BOOTLOADER_JUMP TRUE
#endif

/*
 * I2C fallback driver system settings (consumed by mcuconf.h).
 */
#define SW_I2C_USE_I2C1                     FALSE
#define SW_I2C_USE_I2C2                     FALSE
#define SW_I2C_USE_I2C3                     FALSE
#define SW_I2C_USE_I2C4                     FALSE

