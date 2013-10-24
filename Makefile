-include Makefile.config

TARGETS = src

DIR := $(shell basename `pwd`)

MAKECMDGOALS ?= debug

debug release install uninstall:
	for TRG in $(TARGETS) ; do $(MAKE) -C $$TRG $(MAKECMDGOALS) ; done

clean:
	for TRG in $(TARGETS) ; do $(MAKE) -C $$TRG $(MAKECMDGOALS) ; done
	rm -f core core.* */core */core.*

new: clean all

tar:
	$(MAKE) clean
	cd .. ; tar cfvz $(PROGRAM)-$(VERSION).tar.gz $(DIR);
