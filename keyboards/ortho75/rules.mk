# Ported from STM32F411 to AT32F405 for a compile test.
MCU = AT32F405

# AT32F405RCT7-7: the 'C' is the 256 kB flash density, so link
# against xC (256k flash / 96k RAM) rather than the xB default.
MCU_LDSCRIPT = AT32F405xC

OPT_DEFS += -DCORTEX_ENABLE_WFI_IDLE=TRUE
CFLAGS += -flto=auto
LDFLAGS += -flto=auto
# External EEPROM: 24LC256 on I2C1 (PB6 SCL / PB7 SDA).
# The internal-flash wear-levelling path is then unused; the reason to
# move off it is that AT32 flash erases in whole 2 kB sectors and an
# erase blocks code fetch, which is audible as a hitch while remapping.
EEPROM_DRIVER = i2c
