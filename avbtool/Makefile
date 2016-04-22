CC ?= $(CROSS_COMPILE)gcc
RM = rm -f

INC =
CFLAGS = -c -Wall -ggdb -O2 $(INC) $(EXTRA_CFLAGS)
LDLIBS  = -lm -lrt $(EXTRA_LDFLAGS)

PRG = avbtool

OBJ = $(PRG).o
SRC = $(OBJ:.o=.c)

all: $(PRG)

$(PRG): $(OBJ)

clean:
	$(RM) $(OBJ) $(PRG)
