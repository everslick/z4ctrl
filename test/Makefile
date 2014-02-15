-include ../Makefile.config

TARGET  = z4remote

DEFINES = -DVERSION=\"$(VERSION)\"

CFLAGS += -std=gnu99 -g3 -O0
LFLAGS += -pthread

sources = $(wildcard *.c)
objects = $(subst .c,.o,$(sources))

depend  = .depend

$(depend): Makefile
	$(CC) -MM $(CFLAGS) $(sources) > $@

debug: $(depend) $(objects)
	$(CC) $(objects) $(LFLAGS) -o ../bin/$(TARGET)

release install uninstall doc:

clean:
	cd ../bin ; rm -f $(TARGET)
	rm -f *.o $(depend)

.c.o:
	$(COMPILE.c) $(DEFINES) $(CFLAGS) -c $< $(OUTPUT_OPTION)

-include $(depend)
