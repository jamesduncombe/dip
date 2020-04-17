#
# Dip emulator/interpreter
#
CC = cc
CFLAGS = -Wall -std=c11
LDFLAGS = -lSDL2 -lSDL2_gfx

SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

all: dip

dip: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o dip

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f dip
	rm -f $(OBJS)

.PHONY: clean