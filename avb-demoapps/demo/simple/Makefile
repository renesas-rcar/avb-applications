# TOP_DIR :=
# CROSS_COMPILE :=
# SYSROOT :=
# INSTALL_DIR :=

##############################################################

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
CP := cp
RM := rm -f

##############################################################

DEMO_COMMON_DIR := ../common

LIBS := m
LIBS += rt
LIBS += eavb
LIBS += avtp
LIBS += msrp

CFLAGS := -Wall
CFLAGS += -c
CFLAGS += -g
CFLAGS += -O2
CFLAGS += -std=gnu99
CFLAGS += -I$(DEMO_COMMON_DIR)
CFLAGS += -I$(TOP_DIR)/lib/eavb
CFLAGS += -I$(TOP_DIR)/lib/msrp
CFLAGS += -I$(TOP_DIR)/lib/avtp
CFLAGS += --sysroot=$(SYSROOT)
CFLAGS += $(EXTRA_CFLAGS)

LFLAGS := -pthread
LFLAGS += --sysroot=$(SYSROOT)
LFLAGS += -L$(TOP_DIR)/lib/eavb
LFLAGS += -L$(TOP_DIR)/lib/msrp
LFLAGS += -L$(TOP_DIR)/lib/avtp
LFLAGS += $(addprefix -l,$(LIBS))

#############################################################

OBJS    := packet.o
OBJS    += $(DEMO_COMMON_DIR)/eavb_device.o

HDRS    := $(OBJS:.o=.h) config.h

#############################################################

TARGET1 := simple_talker
OBJS1   := simple_talker.o $(OBJS) $(DEMO_COMMON_DIR)/netif_util.o $(DEMO_COMMON_DIR)/clock.o
HDRS1   := simple_talker.h $(HDRS) $(DEMO_COMMON_DIR)/netif_util.h $(DEMO_COMMON_DIR)/clock.h

#############################################################

TARGET2 := simple_listener
OBJS2   := simple_listener.o $(OBJS) $(DEMO_COMMON_DIR)/stats.o
HDRS2   := simple_listener.h $(HDRS) $(DEMO_COMMON_DIR)/stats.h

#############################################################

all: $(TARGET1) $(TARGET2)

%.o : %.c $(HDRS1) $(HDRS2)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET1) : $(OBJS1)
	$(CC) $^ -o $@ $(LFLAGS)

$(TARGET2) : $(OBJS2)
	$(CC) $^ -o $@ $(LFLAGS)

install: $(TARGET1) $(TARGET2)
	mkdir -p $(INSTALL_DIR)
	install $(TARGET1) $(TARGET2) $(INSTALL_DIR)

clean:
	$(RM) $(OBJS1) $(OBJS2)
	$(RM) $(TARGET1) $(TARGET2)
