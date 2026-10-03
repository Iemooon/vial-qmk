# AT32F405 移植交接（2026-10-03，从 Windows 本机转服务器）

分支 `vial-qmk-AT32`，最新提交 `da1035c17c`。本文只写**已实测确认**的内容，推断项会明确标注。

---

## 1. 结论先说：8K 能不能靠 AT32F405 自己做、要不要外接 PHY / EEPROM

| 问题 | 答复 | 依据 |
|---|---|---|
| 需要外置 USB HS PHY（USB3300）吗 | **不需要，而且没有接口可接** | RM V2.02 p390「OTGHS模块由 OTGHS controller、内置物理层（2.0 HS PHY）以及独立4096字节SRAM组成」；p391 表21-2 中 `OTGHS_D-/D+` 标注「OTGHS 专用管脚」，全表无 ULPI 的 8 位数据＋STP/DIR/NXT＋CLK |
| 需要外接 EEPROM 吗 | **容量上不需要**，256K 内置余量 212KB | 实测 FLASH 36,660B/262,144B＝14.0%，见 §7 |
| 现在这块板已经能跑 8K 吗 | **还不能** | 缺 `report_interval` 层；HS 枚举未上机验证，见 §8、§9 |

硬件硬约束（选型不可违）：

- **晶振必须 12MHz**：RM p391「OTGHS PHY 需要的 12M 时钟由芯片的 HEXT 提供」。8M/25M 通用件不行。
- 用 OTGHS 时 **AHB > 30MHz**；须先等 PHY 12MHz 稳定才能操作 `PWRDOWN`，解除后**再等 1ms**。
- **只能用 AT32F405**：RM p389「AT32F402 仅支持此模块内的 OTGFS」⇒ F402 做不了 HS，不能降成本。
- 设备模式下 1 个双向控制端点＋7 IN＋7 OUT（p390），HS 中断端点数量够用。
- HS 的 OTG 控制脚 SOF PA4／VBUS PB13／ID PB12／OE PC9，本分支矩阵已避开。

---

## 2. 仓库配置（服务器 clone 后必须还原的拓扑）

```
主仓   https://github.com/Iemooon/vial-qmk
  分支 vial-qmk-AT32        HEAD da1035c17c（基点 dd43959ae5 = origin/vial）
  上游 origin/vial = vial-kb/vial-qmk
子模块 lib/chibios-contrib → https://github.com/Iemooon/ChibiOS-Contrib
  分支 at32-f405-hs        commit 5aacc821f7385131f5cfe8abdcaa56cbcf7f775a
  （.gitmodules 的 url 与 branch 都已改写指向该 fork）
子模块 lib/chibios         仍指 qmk/ChibiOS @ 8bd61b80（未改，无补丁）
```

`5aacc82` 只有两处内容：`at32_registry.h` 给 Cx/Rx 各补一行 `AT32_OTG2_SUPPORTS_HS TRUE`，＋新增 `AT32F402xB.ld`。**这个 fork 不能省**：contrib 是 submodule，补丁若只留在工作树，主仓只会显示 `M lib/chibios-contrib`，别人 clone 拿到的是干净的 3ac181e，必然在我卡过的位置失败。

参考树（非依赖）：`D:\Projects\qmk-keychron-2025q3` = Keychron fork，chibios `41e112ce`／contrib `58047eb`，其 `.gitmodules` 同样把 lib/chibios 与 lib/chibios-contrib **双双**指向 `github.com/Keychron/{ChibiOS,ChibiOS-Contrib}`——这正是我采用的做法来源。服务器上没有这份树的话，从 GitHub 取 `Keychron/qmk_firmware` 2025q3 分支即可。

提交链（便于回溯）：

```
da1035c17c  Prune upstream keyboards, keep ortho75 only
8b5da74435  Give PB6/PB7 the alternate function so I2C1 actually owns the pads
b5c09f065d  WIP: select the external-EEPROM (I2C1 + 24LC256) path
c5c8396d0b  Document the AT32 build entry point and its two traps
0049a24aa6  Add build.cmd: AT32 builds need a bash-wrapped entry point
130a55410d  Link AT32F405 at the C density and derive flash size from MCU_LDSCRIPT
4fb84e02bd  Add ortho75 AT32F405 port and a lem0n board skeleton
e12a282147  Wire AT32F405 into the ChibiOS platform layer
```

---

## 3. 环境依赖清单

本机（Windows）实测可用组合：

| 组件 | 位置／版本 | 备注 |
|---|---|---|
| MSYS2 | `C:\msys64`，make 4.4.1 | 提供 sh/tr/sed/printf/uname/GNU find |
| ARM 工具链 | `D:\Projects\arm-none-eabi`，gcc 15.3.1 | **不进系统 PATH**，由 build.cmd 注入 |
| QMK CLI | pip `qmk` 1.2.0，`C:\Python\Python313\Scripts\qmk.exe` | 需要它生成 `genrules`，勿用 make 裸跑 |
| dfu-suffix | `pacman -S mingw-w64-x86_64-dfu-util`（0.11） | `at32-dfu` 签名步骤要用，缺则 `Error 127` |
| python3 | `mklink /H C:\Python\Python313\python3.exe …\python.exe` | 见 §4 坑 3 |

**服务器上（Linux）大概率更简单**：sh/tr/sed/find 本来就在 PATH，`make`、`gcc-arm-none-eabi`（或 15.3.Rel1）、`qmk`（pip）、`dfu-util`（apt/pacman）、`python3` 装上即可。关键是下面 §4 的第一条坑在 Linux 上并不存在，别把它当成 QMK 的缺陷。

`qmk doctor` 可以自检；另外首次要 `git submodule update --init --recursive`，并确保能访问 `Iemooon/ChibiOS-Contrib`（public，无需凭据）。

---

## 4. 踩过的坑，按"会再踩一次"的概率排序

**坑 1 — AT32 链在裸 PowerShell/cmd 下必然失败，且报错极具误导性**

症状：先是一堆 `'tr' 不是内部或外部命令`，补完 PATH 之后变成

```
/bin/sh: -c: line 1: unexpected EOF while looking for matching `"'
builddefs/build_keyboard.mk:303: *** Platform not defined.  Stop.
```

看着像板配置写坏了，**其实不是**。根因：QMK 用 `SHELL=sh`，而 sh 被 Win32 父进程调用时 `-c` 的命令行在约 8KB 处被截断（实测 8000 字节通过／12000 失败），AT32 链单条 gcc 命令展开 >12KB，引号被切在半截。补 PATH 治不了。

解法：把 make 包进 MSYS2 进程树。树根 `build.cmd <keyboard> <keymap>` 就是这层包装，已入库。Linux 上无此限制。

**坑 2 — 增量构建的旧 `.o` 会让配置错误"看起来是好的"**

我改了链接脚本／密度宏之后构建仍报 `[OK]`，实际是复用了早期状态的 `hal_efl_lld.o`。改这类只影响编译期 `#if` 分支的东西时，必须强制重编或清 `.build`，否则"编过了"不构成证据。

**坑 3 — `python3` 命中 Windows 商店占位 stub**

`%LOCALAPPDATA%\Microsoft\WindowsApps\python3.exe` 只是个重定向器，`build_vial.mk:39` 调它得到毫无信息量的退出码 49。修法是**同目录硬链接**：

```
mklink /H C:\Python\Python313\python3.exe C:\Python\Python313\python.exe
```

复制到别处不行——解释器按自身目录定位 `python313.dll`。

**坑 4 — 宏名不能凭记忆写（我连栽三次）**

- `AT32_LDOOVSEL_LEV3` 在这棵树不存在，未定义标识符求值为 0，而 `AT32_LDOOVSEL_1P0V` 恰是 0 ⇒ 掉进 1.0V 分支，报"bus clock exceeding maximum frequency"。正确写法 `AT32_LDOOVSEL_1P3V`（216MHz HCLK 正是 1.3V 档上限）。
- `QMK_MCU_SERIES_AT32F405XX` 是我凭空造的，QMK 里没有这宏 ⇒ 我加的分支永远不可达，后来删掉。
- `$(lastword $(subst x, ,AT32F405xC))` 得到 `C` 而不是 `xC`，与 `xC` 比对永不命中。密度判定改用 `findstring`。

**坑 5 — 密度 ≠ 封装，两个维度别再混**

- `MCU_LDSCRIPT` 的 `xB/xC` 是 **flash 密度**：`AT32F405xB.ld`=128k flash/64k RAM，`AT32F405xC.ld`=256k/96k。
- registry 的 `Kx/Cx/Rx` 是 **封装**，决定 GPIOC/GPIOD/SPI2/UART8 有无。
- 料号 `AT32F405RCT7-7`：R=LQFP64、C=256KB ⇒ package 宏 `AT32F405Rx`（在 board.h）、ld `AT32F405xC`（在 rules.mk）。
- 用 xC 的硬证据：`.heap` 从 53,464B 涨到 86,232B，且 `0x20002F28 + 86,232 = 0x20018000` 正好是 96k ram0 的顶。

**坑 6 — contrib 的 registry 缺陷（已修，但服务器上要认这个 gitlink）**

`AT32F405xx/at32_registry.h` 把整张能力表关在 package 宏里（行 32/212/394）且**没有 #else 兜底**；`AT32_OTG2_SUPPORTS_HS` 只在 Kx 块（192 行），Cx（375）／Rx（558）缺 ⇒ `hal_usb_lld.c:823` 落进 `#else` 调用不存在的 `crmEnableOTG_FS2`，编译断；即使侥幸有该函数也会静默按全速枚举。判 R/C 包同样有 HS 的依据：CMSIS `at32f402_405{kx,cx,rx}.h` 三封装头都定义 `USB_OTG_HS_PERIPH_BASE = 0x40040000U`，OTGHS 命中数均 40。

**坑 7 — AT32 在 ChibiOS 里把引脚交给外设的机制**（做 I2C/SPI/USART 必踩）

I2Cv2 的 `i2c_lld_start()` **完全不碰 GPIO/AF**，只管时钟和 DMA。复用号来自 mode 字的位域：

```c
uint32_t muxr = (mode & PAL_AT32_ALTERNATE_MASK) >> 7;   // bit7-10 = MUX 号
PAL_AT32_MODE_ALTERNATE = (2U << 0U);   PAL_AT32_ALTERNATE(n) = (n) << 7U
```

board.h 的正确写法（**照抄官方板 `AT_START_F405`，别自己造**）：

```c
#define PIN_MODE_MUX(n)   (2U << ((n) * 2U))
#define PIN_MUX(n, v)     ((v) << (((n) % 8U) * 4U))
```

**只写 `PIN_MUX` 而 MODE 留 `PIN_MODE_INPUT` ⇒ 编译干净、线上无波形**——这是我在 `b5c09f065d` 上过一次、`8b5da74435` 才修好的错。

**坑 8 — 一条方法学错误**：`git ls-files <ref> -- <submodule>` 不能用来读远端分支的 gitlink，`ls-files` 只读本地索引。我当时据此得出"对方分支已含我的补丁"，结论作废。读 commit 内容用 `git ls-tree <ref>` / `git show <ref>:<path>`。

---

## 5. 本分支改了什么（相对 origin/vial）

平台接线层：

- `lib/python/qmk/constants.py` — `CHIBIOS_PROCESSORS` 加 `AT32F402`/`AT32F405`（否则 `qmk compile` 直接 "Unknown MCU"）。**注意上游已有 `AT32F415`，缺的只是型号。**
- `platforms/chibios/mcu_selection.mk` — 两个 series 的 `MCU_SERIES`／启动文件／`MCU_LDSCRIPT ?=`。
- `platforms/chibios/platform.mk` — 按 `MCU_LDSCRIPT` 用 `findstring` 推导 `WEAR_LEVELING_EFL_FLASH_SIZE`（xA/xB/xC → 65536/131072/262144），未知密度 `$(error)` 硬停。
- 新板目录 `platforms/chibios/boards/GENERIC_AT32_F405XX/`（board.c/board.h/board.mk/configs/{config,mcuconf,halconf}.h）与 `GENERIC_AT32_F402XX/`。
- `wear_leveling_efl.c` 加 `WEAR_LEVELING_EFL_FLASH_SIZE` 分支（AT32 EFL 无运行时 flash-size 寄存器）；`analog.c`/`spi_master.c`/`ws2812_pwm.c`/`ws2812_spi.c`/`chibios_config.h` 加 AT32 guard。
- mcuconf 里 34 个 `AT32_IRQ_*_PRIORITY` 必须齐（缺一个 `at32_isr.c` 就断），取值抄 contrib 自带 demo `RT-AT-START-F405/cfg/mcuconf.h`。
- USB：`AT32_USB_USE_OTG1=FALSE`／`USE_OTG2=TRUE`／`AT32_USE_USB_OTG2_HS=TRUE`／`AT32_CLOCK48_REQUIRED=TRUE`／`AT32_USB_OTG2_RX_FIFO_SIZE=1024`；board.h **不定义** `BOARD_OTG2_USES_ULPI`；board.h `#define AT32F405Rx`、`AT32_HEXTCLK = 12000000`。

键盘层：`keyboards/ortho75/` 从 STM32F411 改成 AT32F405，`MCU_LDSCRIPT = AT32F405xC`、`EEPROM_DRIVER = i2c`＋`EEPROM_I2C_24LC256`；vial keymap 用 `VIAL_KEYBOARD_UID {0xA0,0xED,0xAC,0x09,0x5F,0xE4,0xCB,0x40}`、4 层。

---

## 6. 引脚现状

矩阵（15 列 × 5 行，均取自 board.h 声明的 51 个 GPIO-capable 脚）：

```
cols: A0 A1 A2 A3 A5 A6 A7 A8 A9 A10 A13 A14 A15 B0 B1 F4 F5 F6 F7   （前15）
rows: B2 B3 B4 B5 F11
```

（注：AT32F405 R 包**没有 GPIOH**、GPIOD 只有 D2，所以 F411 原来的 A10/A2/A1/C15…/B15 那套**不能照搬**。）

I2C1（外接 EEPROM）：`PB6 = SCL`、`PB7 = SDA`，MUX 号 **2**（RM p95 表6-2，列头 MUX0…MUX7；用 PB8/PB9 同为 MUX4 交叉验证过列位）。两脚均为 `PIN_MODE_MUX` ＋ `PIN_MUX(…, 2U)`。

尚未使用、可用于 LED/编码器：`B8 B9 B10 B12 B13`、`C0…C15`、`D2`、`A4 A11 A12` 是 HS 专用或已避开——**A4=OTGHS_SOF、A11=DM、A12=ID、PC9=OE，别拿去扫键**。

---

## 7. EEPROM 预算（内置 flash 方案的实测数）

```
.vectors   512   .text 33,000   .rodata 3,148        → FLASH 36,660 B / 262,144 = 14.0%
.mstack 1,024  .pstack 2,048  .data 1,132  .bss 8,040 → RAM 静态 12,244 B / 98,304 = 12.5%
```

留给 wear leveling 8K 后，代码区仍余 **217,292B ≈ 212KB**。`dynamic keymap = 层×行×列×2 = 4×5×15×2 = 600B`。扇区 2048B（RM p96：F405 系列 2K/扇区），可选档：

| backing | 扇区 | logical |
|---|---|---|
| 8192（QMK 默认） | 4 | 4096 |
| **12288（= c1_pro_8k 取值）** | 6 | 6144 |
| 16384 | 8 | 8192 |

`BACKING_STORE_WRITE_SIZE` 对 AT32 家族 QMK 已内置为 2。外接的唯一实际收益是回避"擦除以整 2K 扇区为单位、擦除期阻塞取指 ⇒ 改键瞬间卡顿"，而该问题的正解是**加大 backing**（页数多则不易写满触发整块搬迁），不必为此多焊一颗芯片。当前分支按你上一轮指示停在**外接 I2C 版**；要退回内置只需删 `EEPROM_DRIVER`/`EEPROM_I2C_24LC256`/halconf 的 `HAL_USE_I2C`/mcuconf 的 `I2C1`/PB6-PB7 的 MUX 五处。

---

## 8. 关键机理：为什么 HS 一通，8K 的描述符其实不用改

两棵树的端点都写 `.PollingIntervalMS = USB_POLLING_INTERVAL_MS`（默认 **1**）。这个字节在 **FS 下＝1 帧＝1ms＝1000Hz**，在 **HS 下＝1 微帧＝125µs＝8000Hz**（HS 中断端点周期为 2^(b−1) 微帧）。所以：

> 只要设备真按 HS 枚举，现有 bInterval=1 自动给出 8K，描述符一个字都不用动。

我早前记的"上游 QMK 封死 1000Hz（`USB_POLLING_INTERVAL_MS` 整数毫秒）"那个结论，**只在 FS 前提下成立**，现已据此更正。

---

## 9. 待办：8K 最后一环是 `report_interval` 层

vial-qmk 里这个符号 **0 命中**。Keychron 的实现分布在**四个文件**（已逐处定位）：

| 文件 | 行 | 内容 |
|---|---|---|
| `keyboards/keychron/common/usb_report_rate.c` | 42,46,47,51,62 | `report_rate_div`；`update_usb_report_interval(&USB_DRIVER, (0x01U << div) - 1)`；存 `EECONFIG_BASE_HSUSB_REPORT_RATE`，`div > 6` 归 0 |
| `tmk_core/protocol/chibios/usb_main.c` | 56,352,362,380,403,442 | `usb_report_interval`；每端点计数 `if (isp->report_interval_count < usbp->report_interval[EP]) ++…`；`update_usb_report_interval()` 定义与枚举时重下发 |
| `lib/chibios/os/hal/src/hal_usb.c` | 519 | `if (ep != 0 && isp->report_interval_count < usbp->report_interval[ep])` ⇒ **发送门控**（注意在 chibios 不在 contrib） |
| `lib/chibios-contrib/os/hal/ports/AT32/LLD/OTGv1/hal_usb_lld.h` | 291,512 | `uint8_t report_interval_count;` ＋ `report_interval[USB_MAX_ENDPOINTS]` |

两树同文件行数差：`hal_usb_lld.c` Δ117、`usb_main.c` Δ66、`hal_usb_lld.h` Δ24、`hal_usb.c` Δ7，**总量约 200 行**（比我先前说的"几十行"大，已更正）。

语义务必看清：这是**降速分频器**。`div=0 → interval=0 → 永不跳过 → 每个轮询周期都发`（HS 下＝8K）；`div=3 → interval=7 → 每 8 周期发 1 次`＝1K。所以它不是"实现 8K"，而是"在 HS 已给的 8K 底座上允许降档"。

`keyboards/keychron/common/usb_report_rate.c` 本分支已随键盘裁剪删掉，源码在 `D:\Projects\qmk-keychron-2025q3`（或 GitHub 的 Keychron fork）。

**唯一无法靠读代码闭环的一项**：主机是否真按 125µs 轮询。上机抓 `lsusb -v`／USBlyzer，看协商到的速度与端点实际周期。附带风险：两棵树的 QMK **都不实现 Device Qualifier / Other-Speed Configuration**——`USB_DESCRIPTOR_DEVICE_QUALIFIER`(6U)／`OTHER_SPEED_CFG`(7U) 只在 ChibiOS `hal_usb.h:68/69` 定义了类型码，`usb_main.c` 与 `usb_descriptor.c` 对 qualifier 的引用均为 0。按 USB 2.0 §9.2.6.1，HS capable 设备本应提供这两个描述符。Keychron 同样没有却量产跑 8K，**由此推断**仅凭 reset 后的 chirp K/J 握手即可枚举成 HS；此为推断、无实测。稳妥做法是移植时顺手补上 qualifier，消除对主机容忍度的依赖。

---

## 10. c1_pro_8k 那组参数（你说供参考，这是取舍结论）

- `dip_switch.pins ["B1"]` — **不要**。配合 `"features": {"dip_switch": true}`，在 `c1_pro_8k.c:23` 的 `dip_switch_update_kb()` 消费，是 Keychron 有线/蓝牙双模的物理开关。
- `indicators.caps_lock "B10"` — 可选，与 8K 无关。注意它不走 QMK 的 LED 矩阵，而是 `c1_pro_8k.c:51` 自己轮询 `host_keyboard_led_state().caps_lock` 再 `gpio_write_pin()`。
- `build.debounce_type "custom"` ＋ `debounce 30` — 字段在本树合法（`keyboard.jsonschema:260`、`info_rules.hjson:22`→`DEBOUNCE_TYPE`）。ortho75 现吃默认 `sym_defer_g`；125µs 周期下去抖策略直接决定延迟下限，而 30ms 是 1000Hz 时代的值。**但 Keychron 树里找不到它的 custom 实现**（键盘目录与 quantum 层均 0 命中），不抄看不到的实现；与 §9 同期按实测手感定。
- `eeprom.wear_leveling logical 6144 / backing 12288` — 若走内置方案，这组值本身合理（6 个 2K 扇区），可直接作为起点。

---

## 11. 复现命令

```bash
git clone --recursive https://github.com/Iemooon/vial-qmk -b vial-qmk-AT32
cd vial-qmk
# Windows：build.cmd ortho75 vial
# Linux  ：qmk compile -kb ortho75 -km vial
```

产物基准（外接 EEPROM 版）：`ortho75_vial.elf 73,748B`／`.bin 36,756B`／`.hex 103,392B`；内置 flash 版：`.bin 36,588B`。
