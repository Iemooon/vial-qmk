/* Copyright 2022 JasonRen(biu)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#define NO_ACTION_ONESHOT

/* Ported from STM32F411 to AT32F405 for a compile test.
 * The external oscillator is declared in the board header as AT32_HEXTCLK
 * and the PLL targets are in GENERIC_AT32_F405XX/configs/mcuconf.h, so the
 * STM32-only STM32_HSECLK macro is intentionally not carried over. */
