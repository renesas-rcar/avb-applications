TOP_DIR ?=$(shell pwd)

SYSROOT            ?=/opt/poky/2.0.1/sysroots/aarch64-poky-linux
CROSS_COMPILE      ?=/opt/poky/2.0.1/environment-setup-aarch64-poky-linuxi-
INSTALL_DIR        ?=$(shell pwd)/demo/sample

export TOP_DIR CROSS_COMPILE SYSROOT INSTALL_DIR

subdirs := lib mrpdummy avblauncher demo
include $(TOP_DIR)/Makefile.include
