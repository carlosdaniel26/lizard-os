OUTPUT := kernel
BIN := build/bin

include makefiles/emulation.mk

.PHONY: all lizard nolibc runtime clean distclean

all: lizard nolibc runtime
	@mkdir -p $(BIN)
	$(CC) $(LDFLAGS) $(shell find build -name "*.o") -o $(BIN)/$(OUTPUT)

lizard:
	$(MAKE) -C lizard

nolibc:
	$(MAKE) -C nolibc

runtime:
	$(MAKE) -C runtime

clean:
	rm -rf build

distclean: clean
	rm -rf lizard/limine.h

gdb:
	gdb -tui -ex "target remote :1234" -x script.gdb
