TARGET = getscancodes
SOURCES = getscancodes.c

CFLAGS=-O2 -Wall
CC=gcc

OBJECTS=$(SOURCES:.c=.o)

all: $(TARGET) evdev_map xi2watch

cc:
	$(MAKE) CC=cc \
		all

$(TARGET): $(OBJECTS)
	$(CC) -o  $@ $(OBJECTS)

clean:
	$(RM) $(TARGET) $(OBJECTS) evdev_map key_names.h xi2watch

key_names.h: $(FORCE)
	gcc -E -dM -x c - <<< '#include <linux/input-event-codes.h>' | \
	   perl -ne 'if (/^\#define (KEY_(\w+))\s+\S+/) { print "  { $$1, \"$$2\" },\n" }' \
	   > key_names.h

evdev_map: evdev_map.c key_names.h
	c99 -Wall -Wextra -D_XOPEN_SOURCE=600 -g -O2 -o evdev_map evdev_map.c
xi2watch: xi2watch.c
	gcc -Wall -std=c99 -o xi2watch xi2watch.c -lX11 -lXi
