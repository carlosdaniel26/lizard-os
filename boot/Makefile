# UEFI loader: freestanding C -> PE32+ EFI application, using only stock binutils.
#
# ld -m i386pep links straight to PE. We fix the image base low and strip
# relocations (--disable-dynamicbase --disable-reloc-section) so OVMF loads it
# verbatim at 0x400000 - the "self-relocating .reloc" path that gnu-efi uses is
# not needed and binutils' i386pep .reloc output is rejected by OVMF.

CC := gcc
LD := ld

CFLAGS := -ffreestanding -fno-stack-protector -fno-stack-check -fno-strict-aliasing \
          -fno-builtin -fno-ident -mno-red-zone -fshort-wchar -m64 \
          -fno-pic -fno-pie -mcmodel=small \
          -Wall -Wextra -Wno-unused-parameter -I.. -c

LDFLAGS := -m i386pep -nostdlib --subsystem=10 -e efi_main --no-insert-timestamp -s \
           --disable-dynamicbase --disable-reloc-section --image-base 0x400000

.PHONY: all clean
all: BOOTX64.EFI

main.o: main.c efi.h ../lizard/boot.h
	$(CC) $(CFLAGS) $< -o $@

BOOTX64.EFI: main.o
	$(LD) $(LDFLAGS) -o $@ $<

clean:
	rm -f main.o BOOTX64.EFI
