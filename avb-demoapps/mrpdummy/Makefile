CC=$(CROSS_COMPILE)gcc
RM = rm -f

LIBS := m
LIBS += rt
LIBS += msrp

CFLAGS := -Wall
CFLAGS += -c
CFLAGS += -g
CFLAGS += -O2
CFLAGS += -std=gnu99
CFLAGS += -I$(TOP_DIR)/lib/msrp
CFLAGS += $(EXTRA_CFLAGS)

LFLAGS := -pthread
LFLAGS += -L$(TOP_DIR)/lib/msrp
LFLAGS += $(addprefix -l,$(LIBS))

#############################################################

TARGET := mrpdummy
HDRS   := msrp.h
OBJS   := mrpdummy.o

#############################################################

all: $(TARGET)

#%.o : %.c $(HDRS)
%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET) : $(OBJS)
	$(CC) $^ -o $@ $(LFLAGS)

install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	install $(TARGET) $(INSTALL_DIR)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

