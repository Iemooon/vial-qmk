# Ported from STM32F411 to AT32F405 for a compile test.
MCU = AT32F405

OPT_DEFS += -DCORTEX_ENABLE_WFI_IDLE=TRUE
CFLAGS += -flto=auto
LDFLAGS += -flto=auto