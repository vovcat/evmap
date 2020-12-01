TARGETS = getscancodes evmap xi2watch
SOURCES = $(addsuffix .c,$(TARGETS))
OBJECTS = $(SOURCES:.c=.o) key_names.inc

$(info SOURCES=$(SOURCES))

CC=c99
CFLAGS=-Wall -Wextra -g -O0
LDFLAGS=-g

all: $(TARGETS)
clean:; $(RM) $(TARGETS) $(OBJECTS)

getscancodes: getscancodes.c

key_names.inc: SCRIPT='$$v = int($$3)||oct($$3), printf(qq|  { %s, "%s" }, // %d 0x%x\n|, \
	$$1, $$2, $$v, $$v) if m/^\#define (KEY_(\w+))\s+(\d\S*)/'
key_names.inc: Makefile
	gcc -xc -E -dM - <<< '#include <linux/input.h>' |\
	perl -ne $(SCRIPT) |sort -k6n >$@

evmap.o: key_names.inc
evmap: CFLAGS+=-D_XOPEN_SOURCE=600

xi2watch: LDLIBS+=-lX11 -lXi
xi2watch: xi2watch.c
