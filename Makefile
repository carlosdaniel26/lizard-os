OUTPUT := kernel
BIN := build/bin
export ARCH := x86_64
LDFLAGS := -no-pie -z max-page-size=0x1000 -Wl,--gc-sections -Wl,--build-id=none


.PHONY: all lizard nolibc runtime clean distclean

all: lizard nolibc runtime
	@mkdir -p $(BIN)
	@$(CC) $(LDFLAGS) -T linker-x86_64.ld -nostdlib -no-pie $(shell find build -name "*.o") -o $(BIN)/$(OUTPUT)
	@python3 scripts/build_status.py

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
include makefiles/emulation.mk
