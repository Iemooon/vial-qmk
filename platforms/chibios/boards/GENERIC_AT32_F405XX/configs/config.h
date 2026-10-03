// Copyright 2023-2025 HorrorTroll <https://github.com/HorrorTroll>
// Copyright 2023-2025 Zhaqian <https://github.com/zhaqian12>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/*
 * The F405 high-speed controller is OTG2, so QMK must drive USBD2 rather than
 * the default USBD1 (see tmk_core/protocol/chibios/usb_main.h).
 */
#if !defined(USB_DRIVER)
#    define USB_DRIVER USBD2
#endif

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

/*
 * The AT32 EFL driver has no runtime flash-size register, so
 * wear_leveling_efl.c reads this instead. 128k matches the length in
 * AT32F405xB.ld, which is what this firmware links against.
 */
#define WEAR_LEVELING_EFL_FLASH_SIZE (128 * 1024)
