# TOP_DIR :=
# CROSS_COMPILE :=
# INSTALL_DIR :=

##############################################################

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
CP := cp
RM := rm -f

##############################################################

DEMO_COMMON_DIR := $(TOP_DIR)/demo/common

LIBS := m
LIBS += rt

CFLAGS := -Wall
CFLAGS += -c
CFLAGS += -g
CFLAGS += -O2
CFLAGS += -std=gnu99
CFLAGS += -I.
CFLAGS += -I$(TOP_DIR)/lib/avtp
CFLAGS += -I$(DEMO_COMMON_DIR)
CFLAGS += -Iinih/
CFLAGS += $(EXTRA_CFLAGS)

LFLAGS := -pthread
LFLAGS += $(addprefix -l,$(LIBS))

#############################################################

TARGET := avblauncher
OBJS   := avblauncher.o $(DEMO_COMMON_DIR)/netif_util.o inih/ini.o
HDRS   := avblauncher.h $(DEMO_COMMON_DIR)/netif_util.h inih/ini.h

######################################]#######################

all: $(TARGET)

%.o : %.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET) : $(OBJS)
	$(CC) $^ -o $@ $(LFLAGS)

install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	install $(TARGET) $(INSTALL_DIR)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)
