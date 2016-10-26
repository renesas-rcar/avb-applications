# CROSS_COMPILE :=
INCSHARED ?= $(KERNEL_SRC)/drivers/staging/avb-streaming

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
CFLAGS += -I$(INCSHARED)

#############################################################

TARGET = libeavb.a
OBJS = eavb.o
HDRS = eavb.h

#############################################################

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o : %.c $(HDRS)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJS)
