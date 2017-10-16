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
LIBS += yaml
LIBS += avdecc
LIBS += jdksavdecc-c

CFLAGS := -Wall
CFLAGS += -c
CFLAGS += -g
CFLAGS += -O2
CFLAGS += -std=gnu99
CFLAGS += -I.
CFLAGS += -I$(TOP_DIR)/lib/avtp
CFLAGS += -I$(TOP_DIR)/lib/avdecc
CFLAGS += -I$(TOP_DIR)/lib/avdecc/jdksavdecc-c/include
CFLAGS += -I$(DEMO_COMMON_DIR)
CFLAGS += -Iinih/
CFLAGS += $(EXTRA_CFLAGS)

LFLAGS := -pthread
LFLAGS += -L$(TOP_DIR)/lib/avdecc
LFLAGS += -L$(TOP_DIR)/lib/avdecc/jdksavdecc-c
LFLAGS += $(addprefix -l,$(LIBS))

VPATH += $(TOP_DIR)/lib/avdecc
VPATH += $(TOP_DIR)/lib/avdecc/jdksavdecc-c
#############################################################

TARGET := avblauncher
OBJS   := avblauncher.o $(DEMO_COMMON_DIR)/netif_util.o inih/ini.o libavdecc.a libjdksavdecc-c.a
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
