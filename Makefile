OUTPUT := kernel
BIN := build/bin
export ARCH := x86_64
LDFLAGS := -no-pie -z max-page-size=0x1000 -Wl,--gc-sections -Wl,--build-id=none


.PHONY: all lizard nolibc runtime boot clean distclean

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

boot:
	$(MAKE) -C boot

clean:
	rm -rf build
	$(MAKE) -C boot clean

distclean: clean
	rm -f esp.img ovmf_vars.fd

include makefiles/emulation.mk
