## Freestanding userspace: a tiny libc (lib/ -> libc.a), one ELF per bin/*.c,
## plus the doom/ port. -I .. puts the repo root on the path so <abi/syscall.h>
## resolves; the private headers live in include/.
.PHONY: all clean doom

CC      := gcc
CFLAGS  := -I include -I .. -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
           -fno-stack-check -m64 -std=gnu11 -O2 -Wall -Wextra \
           -ffunction-sections -fdata-sections
LDFLAGS := -nostdlib -no-pie -static -Wl,-Ttext,0x400000 -Wl,--build-id=none \
           -Wl,--gc-sections -e _start

LIBOBJ := lib/crt0.o lib/syscall.o lib/string.o lib/stdlib.o lib/printf.o \
          lib/scanf.o lib/stdio_file.o lib/misc.o lib/math.o lib/errno.o
BINS   := $(patsubst bin/%.c,bin/%,$(wildcard bin/*.c))

all: $(BINS) doom

libc.a: $(LIBOBJ)
	ar rcs $@ $^

bin/%: bin/%.c libc.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< libc.a

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

lib/%.o: lib/%.S
	$(CC) $(CFLAGS) -c $< -o $@

doom: libc.a
	$(MAKE) -C doom

clean:
	rm -f $(LIBOBJ) libc.a $(BINS)
	$(MAKE) -C doom clean
