# Building the AT32F405 port

TL;DR: run `build.cmd <keyboard> <keymap>` from the repo root, e.g.

    build.cmd ortho75 vial

Do **not** run `qmk compile` directly from PowerShell or cmd.

## Why the direct call fails

A bare `qmk compile -kb ortho75 -km vial` from PowerShell or cmd dies with

```
'tr' 不是内部或外部命令 …
'sed' 不是内部或外部命令 …
/bin/sh: -c: line 1: unexpected EOF while looking for matching `"'
builddefs/build_keyboard.mk:303: *** Platform not defined.  Stop.
```

Two separate causes, and fixing only the first one still leaves you stuck:

1. **Missing POSIX utilities.** QMK's makefiles set `SHELL=sh` and use `tr`,
   `sed`, `printf`, `uname`, GNU `find`. Those live in MSYS2's `/usr/bin`,
   which is not on a default Windows PATH.

2. **The 8 kB command-line limit.** Once PATH is fixed, the build still fails,
   now with `unexpected EOF while looking for matching '"'` followed by
   `Platform not defined`. This looks like a broken board configuration but is
   not: when `/bin/sh` is spawned by a Win32 parent process, its `-c` argument
   string is truncated at roughly 8 kB (8000 bytes passes, 12000 fails). The
   AT32 chain expands a single `gcc` command line past 12 kB — the object list
   alone is longer than that — so the quoted command reaches `sh` cut in half
   and the remaining quotes no longer balance.

   Running make inside the MSYS2 process tree avoids the Win32 limit, which is
   exactly what `build.cmd` does: it assembles the PATH and then execs `qmk`
   under `bash -lc`.

The second cause is the one worth remembering: **adding to PATH is not enough,
the invocation has to start from bash.**

## Prerequisites

| Component | Where it lives | How to get it |
|---|---|---|
| MSYS2 (make, sh, coreutils) | `C:\msys64` | `winget install MSYS2.MSYS2`, then `pacman -S make git diffutils` |
| ARM toolchain | `D:\Projects\arm-none-eabi` | gcc 15.3.1; not on the system PATH by design |
| `dfu-suffix` (needed by the `at32-dfu` bootloader step) | `C:\msys64\mingw64\bin` | `pacman -S mingw-w64-x86_64-dfu-util` |
| `python3` | `C:\Python\Python313\python3.exe` | see below |

`dfu-suffix` matters because the last build step signs the image for DFU:

```
sh: line 2: dfu-suffix: command not found
make: *** [builddefs/common_rules.mk:254: .build/ortho75_vial.bin] Error 127
```

Everything compiles and links first, so the port itself is fine — this is only
a missing host tool. `C:\msys64\mingw64\bin` is already on the PATH that
`build.cmd` assembles, so installing the package is the whole fix.

## The `python3` trap

Windows ships an App Execution Alias at
`%LOCALAPPDATA%\Microsoft\WindowsApps\python3.exe` that is only a store
redirector. QMK's `build_vial.mk` calls `python3`, hits that stub, and the Vial
definition header step exits with a meaningless code 49:

```
make: *** [builddefs/build_vial.mk:39: …/vial_generated_keyboard_definition.h] Error 49
```

Fix it with a hardlink in the *same directory* as the real interpreter:

    mklink /H C:\Python\Python313\python3.exe C:\Python\Python313\python.exe

Copying `python.exe` elsewhere does not work — the interpreter locates
`python313.dll` relative to its own directory, so a copy in another folder
fails to start.

## Alternative: use an MSYS2 terminal

Inside `C:\msys64\usr\bin\bash.exe -l` the PATH is already complete, so plain

    cd /d/Projects/vial-qmk && qmk compile -kb ortho75 -km vial

works without the wrapper.

## After changing board configuration

Object files are cached under `.build/`. If you change a linker script, a
density macro or anything else that only affects compile-time `#if` branches in
ChibiOS headers, force a rebuild of the affected translation unit rather than
trusting an incremental build to prove the new configuration compiles — a stale
`.o` will happily keep `[OK]` next to a source file that no longer matches.
When in doubt:

    rm -rf .build/obj_ortho75_vial && build.cmd ortho75 vial

## Current verified state

`build.cmd ortho75 vial` produces, from a clean PATH in both cmd and PowerShell:

    .build/ortho75_vial.elf   74,052 B
    .build/ortho75_vial.bin   36,588 B   (incl. DFU suffix)
    .build/ortho75_vial.hex  102,939 B
    text 32,052  rodata 2,868  data 1,132  bss 7,864  mstack 1,024  pstack 2,048

against `AT32F405xC.ld` (256 kB flash / 96 kB RAM) for an AT32F405RCT7.

## Hardware notes

- **No external USB PHY.** The AT32F405 OTGHS block has an integrated
  high-speed PHY; `BOARD_OTG2_USES_ULPI` is a STM32-ism and must stay
  undefined (see `platforms/chibios/boards/GENERIC_AT32_F405XX/board/board.h`).
  A USB3300 and its 12-wire ULPI bus are not needed.
- **12 MHz crystal.** The HS PHY takes its 12 MHz reference from HEXT
  (`AT32_HEXTCLK` in the board header), so this is a BOM constraint, not a
  software preference.
- **EEPROM.** The port as merged uses internal flash via
  `embedded_flash` wear levelling (`BACKING_STORE_WRITE_SIZE` 2 for the AT32
  family, 8 kB backing / 4 kB logical by default). If a board populates an
  external EEPROM, select it with `EEPROM_DRIVER` instead and leave the
  internal-flash path unused.
