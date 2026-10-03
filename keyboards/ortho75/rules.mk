# Ported from STM32F411 to AT32F405 for a compile test.
MCU = AT32F405

# AT32F405RCT7-7: the 'C' is the 256 kB flash density, so link
# against xC (256k flash / 96k RAM) rather than the xB default.
MCU_LDSCRIPT = AT32F405xC

OPT_DEFS += -DCORTEX_ENABLE_WFI_IDLE=TRUE
CFLAGS += -flto=auto
LDFLAGS += -flto=auto