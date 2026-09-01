## Freestanding userspace. Layout:
##   lib/       the tiny libc - sources (*.c, *.S) plus its build output
##              (lib/*.o, lib/libc.a)
##   src/       one .c per program
##   include/   public headers
##   bin/       one built ELF per src/ program; the disk image is staged from here
## -I .. puts the repo root on the path so <abi/syscall.h> resolves.
.PHONY: all clean doom

CC      := gcc
CFLAGS  := -I include -I .. -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
           -fno-stack-check -m64 -std=gnu11 -O2 -Wall -Wextra \
           -ffunction-sections -fdata-sections
LDFLAGS := -nostdlib -no-pie -static -Wl,-Ttext,0x400000 -Wl,--build-id=none \
           -Wl,--gc-sections -e _start

LIBSRC := $(wildcard lib/*.c lib/*.S)
LIBOBJ := $(patsubst lib/%.c,lib/%.o,$(filter %.c,$(LIBSRC))) \
          $(patsubst lib/%.S,lib/%.o,$(filter %.S,$(LIBSRC)))
BINS   := $(patsubst src/%.c,bin/%,$(wildcard src/*.c))

all: $(BINS) doom

lib/libc.a: $(LIBOBJ)
	ar rcs $@ $^

bin/%: src/%.c lib/libc.a | bin
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< lib/libc.a

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

lib/%.o: lib/%.S
	$(CC) $(CFLAGS) -c $< -o $@

bin:
	mkdir -p bin

doom: lib/libc.a
	$(MAKE) -C doom

clean:
	rm -rf bin
	rm -f lib/*.o lib/libc.a
	$(MAKE) -C doom clean
