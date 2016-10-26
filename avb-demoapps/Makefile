TOP_DIR ?=$(shell pwd)

CROSS_COMPILE      ?=aarch64-poky-linux-
INSTALL_DIR        ?=$(shell pwd)/demo/sample

export TOP_DIR CROSS_COMPILE INSTALL_DIR

subdirs := lib mrpdummy avblauncher demo
include $(TOP_DIR)/Makefile.include
