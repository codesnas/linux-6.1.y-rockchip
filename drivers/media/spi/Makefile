# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -I $(srctree)/drivers/media/dvb-frontends/cxd2880

# Please keep it alphabetically sorted by Kconfig name
# (e. g. LC_ALL=C sort Makefile)
obj-$(CONFIG_CXD2880_SPI_DRV) += cxd2880-spi.o
obj-$(CONFIG_VIDEO_GS1662) += gs1662.o
obj-$(CONFIG_VIDEO_ROCKCHIP_PREISP) += rk1608.o rk1608_dphy.o
rk1608-objs += rk1608_dev.o rk1608_core.o
