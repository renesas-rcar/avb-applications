# CROSS_COMPILE :=

##############################################################

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
RM := rm -f

##############################################################

ARFLAGS = r

CFLAGS  = -Wall
CFLAGS += -c
CFLAGS += -g
CFLAGS += -O2
CFLAGS += -std=gnu99
CFLAGS += --sysroot=$(SYSROOT)

#############################################################

TARGET = libavtp.a
OBJS = avtp.o
HDRS = avtp.h

#############################################################

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o : %.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJS)
