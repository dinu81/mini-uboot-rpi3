#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# SPDX-License-Identifier:	GPL-2.0+
#


obj-y += initcall.o
obj-y += ldiv.o


obj-y += errno.o
obj-y += display_options.o
CFLAGS_display_options.o := $(if $(BUILD_TAG),-DBUILD_TAG='"$(BUILD_TAG)"')
#obj-$(CONFIG_BCH) += bch.o
#obj-y += crc32.o
obj-y += ctype.o
obj-y += div64.o
obj-y += hang.o
obj-y += linux_string.o
obj-y += string.o
obj-y += time.o

# Main U-Boot always uses the full printf support
obj-y += vsprintf.o panic.o strto.o strmhz.o

subdir-ccflags-$(CONFIG_CC_OPTIMIZE_LIBS_FOR_SPEED) += -O2
