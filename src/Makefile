-include ../Makefile.config

TARGET   = $(PROGRAM)

DEFINES  = -DVERSION=\"$(VERSION)\"

LFLAGS  += -L$(PREFIX)/lib -pthread -m64
CFLAGS  += -I$(PREFIX)/include/ -m64

MAKECMDGOALS ?= debug

ifneq ($(MAKECMDGOALS),release)
DEFINES += -DDEBUG
CFLAGS  += -g3 -O0
else
CFLAGS  += -s
endif

headers  = $(wildcard *.h)
sources  = $(wildcard *.c)
objects  = $(subst .c,.o,$(sources))

depend   = .depend

$(depend): Makefile
	$(CC) -MM $(CFLAGS) $(sources) > $@

debug release: $(depend) $(objects)
	$(CC) $(LFLAGS) -o ../bin/$(TARGET) $(objects)

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

clean:
	rm -f ../bin/$(TARGET) *.o $(depend)

.c.o:
	$(COMPILE.c) $(DEFINES) $(CFLAGS) -c $< $(OUTPUT_OPTION)

-include $(depend)
