# CROSS_COMPILE :=

##############################################################

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
RM = rm -f

##############################################################

ARFLAGS = r

LIBS += jdksavdecc-c

CFLAGS  = -Wall
CFLAGS += -c
CFLAGS += -g
CFLAGS += -O2
CFLAGS += -std=gnu99
CFLAGS += -Ijdksavdecc-c/include

LFLAGS += -Ljdksavdecc-c
LFLAGS += $(addprefix -l,$(LIBS))

#############################################################

TARGET = libavdecc.a libjdksavdecc-c.a
OBJS = avdecc.o common.o avdecc_entity.o adp.o aecp.o acmp.o
HDRS = avdecc.h avdecc_internal.h

#############################################################

all: $(TARGET)

libavdecc.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o : %.c $(HDRS) libjdksavdecc-c.a
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS)

libjdksavdecc-c:
	git submodule init
	git submodule update
	cd jdksavdecc-c; cmake -G "Unix Makefiles"

libjdksavdecc-c.a: libjdksavdecc-c
	cd jdksavdecc-c; make

clean:
	$(RM) $(TARGET) $(OBJS)
	cd jdksavdecc-c; make clean
