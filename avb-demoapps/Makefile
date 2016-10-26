TOP_DIR ?=$(shell pwd)

INCSHARED          ?=$(KERNEL_SRC)/drivers/avb-streaming
CROSS_COMPILE      ?=aarch64-poky-linux-
INSTALL_DIR        ?=$(shell pwd)/demo/sample

export TOP_DIR CROSS_COMPILE INSTALL_DIR

subdirs := lib mrpdummy avblauncher demo
include $(TOP_DIR)/Makefile.include
