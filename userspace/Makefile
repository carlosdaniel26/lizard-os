## Freestanding userspace. Layout:
##   lib/       the tiny libc - sources only (*.c, *.S, *.h)
##   src/       one .c per program
##   include/   public headers
##   build/     libc objects + libc.a (generated)
##   bin/       one built ELF per src/ program; the disk image is staged from here
## -I .. puts the repo root on the path so <abi/syscall.h> resolves.
.PHONY: all clean doom

CC      := gcc
CFLAGS  := -I include -I .. -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
           -fno-stack-check -m64 -std=gnu11 -O2 -Wall -Wextra \
           -ffunction-sections -fdata-sections
LDFLAGS := -nostdlib -no-pie -static -Wl,-Ttext,0x400000 -Wl,--build-id=none \
           -Wl,--gc-sections -e _start

BUILD  := build

LIBSRC := $(wildcard lib/*.c lib/*.S)
LIBOBJ := $(patsubst lib/%.c,$(BUILD)/%.o,$(filter %.c,$(LIBSRC))) \
          $(patsubst lib/%.S,$(BUILD)/%.o,$(filter %.S,$(LIBSRC)))
BINS   := $(patsubst src/%.c,bin/%,$(wildcard src/*.c))

all: $(BINS) doom

$(BUILD)/libc.a: $(LIBOBJ)
	ar rcs $@ $^

bin/%: src/%.c $(BUILD)/libc.a | bin
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(BUILD)/libc.a

$(BUILD)/%.o: lib/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: lib/%.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bin $(BUILD):
	mkdir -p $@

doom: $(BUILD)/libc.a
	$(MAKE) -C doom

clean:
	rm -rf bin $(BUILD)
	$(MAKE) -C doom clean
