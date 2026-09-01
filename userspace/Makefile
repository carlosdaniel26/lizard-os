## Freestanding userspace: a tiny libc (lib/ -> libc.a) plus one ELF per bin/*.c.
## -I .. puts the repo root on the path so <abi/syscall.h> resolves; the private
## headers live in include/.
.PHONY: all clean

CC      := gcc
CFLAGS  := -I include -I .. -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
           -fno-stack-check -m64 -std=gnu11 -O2 -Wall -Wextra \
           -ffunction-sections -fdata-sections
LDFLAGS := -nostdlib -no-pie -static -Wl,-Ttext,0x400000 -Wl,--build-id=none \
           -Wl,--gc-sections -e _start

LIBOBJ := lib/crt0.o lib/syscall.o lib/string.o lib/puts.o lib/printf.o
BINS   := $(patsubst bin/%.c,bin/%,$(wildcard bin/*.c))

all: $(BINS)

libc.a: $(LIBOBJ)
	ar rcs $@ $^

bin/%: bin/%.c libc.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< libc.a

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

lib/%.o: lib/%.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIBOBJ) libc.a $(BINS)
