.PHONY: run debug gdb ddd _debug-qemu hdd esp

HDD        := hda.img
HDD_SIZE   := 64
HDD_LABEL  := LIZARD
HDD_PART_LBA := 2048
HDD_PART_OFFSET := $(shell expr $(HDD_PART_LBA) \* 512)

ESP        := esp.img
ESP_SIZE   := 48
ESP_LABEL  := LIZEFI
LOADER     := boot/BOOTX64.EFI

## UEFI firmware (OVMF). VARS is copied to a writable per-tree file. Distros
## disagree on the path: Debian/Ubuntu ship /usr/share/OVMF/OVMF_{CODE,VARS}_4M.fd,
## Arch/Fedora ship /usr/share/edk2/x64/OVMF_{CODE,VARS}.4m.fd. Pick the first hit.
OVMF_CODE  := $(firstword $(wildcard \
	/usr/share/OVMF/OVMF_CODE_4M.fd \
	/usr/share/edk2/x64/OVMF_CODE.4m.fd \
	/usr/share/edk2-ovmf/x64/OVMF_CODE.fd \
	/usr/share/OVMF/OVMF_CODE.fd) /usr/share/OVMF/OVMF_CODE_4M.fd)
OVMF_VARS  := ovmf_vars.fd
OVMF_VARS_SRC := $(firstword $(wildcard \
	/usr/share/OVMF/OVMF_VARS_4M.fd \
	/usr/share/edk2/x64/OVMF_VARS.4m.fd \
	/usr/share/edk2-ovmf/x64/OVMF_VARS.fd \
	/usr/share/OVMF/OVMF_VARS.fd) /usr/share/OVMF/OVMF_VARS_4M.fd)

## /sbin is usually off a non-root PATH, so resolve these host tools explicitly.
MKFS_FAT  := $(shell command -v mkfs.fat 2>/dev/null || echo /sbin/mkfs.fat)
SFDISK    := $(shell command -v sfdisk 2>/dev/null || echo /sbin/sfdisk)
MCOPY     := $(shell command -v mcopy 2>/dev/null || echo /usr/bin/mcopy)
MMD       := $(shell command -v mmd 2>/dev/null || echo /usr/bin/mmd)

## Userspace programs staged into the FAT16 root. lizard/main.c spawns /sh as
## the first user task; /hello and /argtest are throwaway test programs
## (argtest echoes its argv). All must be ET_EXEC ELFs with 8.3-safe names.
SH      := userspace/bin/sh
HELLO   := userspace/bin/hello
ARGTEST := userspace/bin/argtest

## doomgeneric port: ELF + IWAD, both copied into the image root. The port is a
## patch over an upstream clone (userspace/doom/), launched with `-iwad
## /doom1.wad`. First build of it needs network.
DOOM_ELF := userspace/doom/doom
DOOM_WAD := userspace/doom/wad/doom1.wad

USERSPACE_SRC := $(wildcard userspace/lib/*.c userspace/lib/*.S userspace/lib/*.h \
                            userspace/src/*.c \
                            userspace/include/*.h userspace/include/sys/*.h abi/*.h) \
                 userspace/Makefile userspace/doom/Makefile userspace/doom/doomgeneric.patch

$(SH) $(HELLO) $(ARGTEST) $(DOOM_ELF): $(USERSPACE_SRC) userspace/Makefile
	$(MAKE) -C userspace

## IWAD, staged as doom1.wad. Fetched once from the Akbar30Bill/DOOM_wads repo;
## override DOOM_WAD_URL to stage a different one (doom2.wad, plutonia.wad, ...).
DOOM_WAD_URL ?= https://raw.githubusercontent.com/Akbar30Bill/DOOM_wads/master/doom1.wad
$(DOOM_WAD):
	@mkdir -p $(dir $@)
	@echo "fetching $(notdir $(DOOM_WAD_URL)) (one-time)..."
	curl -fsSL -o $@ $(DOOM_WAD_URL)

## ---- Root disk image (MBR + FAT16 partition) --------------------------------
## The kernel boots with root=ata0p0, so the image needs a real MBR partition
## table. Layout: one bootable FAT16 (type 0x0e) partition starting at LBA 2048,
## spanning the rest of the disk. lizard/fat16.c reads the BPB from LBA 0 of the
## *partition* block device (blk_dev_part_read adds the partition start), which
## is where mkfs.fat --offset writes it.

$(HDD): $(SH) $(HELLO) $(ARGTEST) $(DOOM_ELF) $(DOOM_WAD)
	@for pair in "$(MKFS_FAT):dosfstools" "$(SFDISK):fdisk" "$(MCOPY):mtools"; do \
		bin=$${pair%:*}; pkg=$${pair##*:}; \
		command -v $$bin >/dev/null 2>&1 || { \
			echo "ERROR: $$(basename $$bin) not found - install it with: sudo apt install $$pkg"; \
			exit 1; }; \
	done
	dd if=/dev/zero of=$@ bs=1M count=$(HDD_SIZE) status=none
	printf 'label: dos\nstart=%s, type=0e, bootable\n' $(HDD_PART_LBA) | $(SFDISK) -q $@
	$(MKFS_FAT) -F 16 -s 4 -S 512 -r 512 -R 1 --offset $(HDD_PART_LBA) -n $(HDD_LABEL) $@
	MTOOLS_SKIP_CHECK=1 $(MCOPY) -i $@@@$(HDD_PART_OFFSET) $(SH) ::/SH
	MTOOLS_SKIP_CHECK=1 $(MCOPY) -i $@@@$(HDD_PART_OFFSET) $(HELLO) ::/HELLO
	MTOOLS_SKIP_CHECK=1 $(MCOPY) -i $@@@$(HDD_PART_OFFSET) $(ARGTEST) ::/ARGTEST
	MTOOLS_SKIP_CHECK=1 $(MCOPY) -i $@@@$(HDD_PART_OFFSET) $(DOOM_ELF) ::/DOOM
	MTOOLS_SKIP_CHECK=1 $(MCOPY) -i $@@@$(HDD_PART_OFFSET) $(DOOM_WAD) ::/DOOM1.WAD

hdd: $(HDD)

## ---- EFI System Partition image -------------------------------------------------
## Raw FAT32 volume (no partition table - OVMF boots it as removable media) with
## the loader at the fallback path and the kernel ELF at the root. boot/BOOTX64.EFI
## is built by `make boot` from the top Makefile.

$(LOADER):
	$(MAKE) -C boot

$(ESP): all $(LOADER)
	dd if=/dev/zero of=$@ bs=1M count=$(ESP_SIZE) status=none
	$(MKFS_FAT) -F 32 -n $(ESP_LABEL) $@
	$(MMD)   -i $@ ::/EFI ::/EFI/BOOT
	$(MCOPY) -D o -i $@ $(LOADER)      ::/EFI/BOOT/BOOTX64.EFI
	$(MCOPY) -D o -i $@ $(BIN)/$(OUTPUT) ::/kernel.elf

esp: $(ESP)

$(OVMF_VARS):
	cp $(OVMF_VARS_SRC) $@

## ---- QEMU --------------------------------------------------------------------

## Kernel console (COM1) is mirrored to the terminal and to KERNEL_LOG.
KERNEL_LOG := kernel_log.txt
SERIAL     := -chardev stdio,id=com1,logfile=$(KERNEL_LOG) -serial chardev:com1

## -M pc keeps the PIIX IDE controller lizard/ata.c drives; hda.img stays IDE
## index 0 so root=ata0p0 is unchanged, the ESP is index 1. OVMF is supplied as
## two pflash units (read-only code + writable vars).
QEMU_UEFI = qemu-system-$(ARCH) -M pc \
	-drive if=pflash,format=raw,unit=0,readonly=on,file=$(OVMF_CODE) \
	-drive if=pflash,format=raw,unit=1,file=$(OVMF_VARS) \
	-drive file=$(HDD),format=raw,if=ide,index=0 \
	-drive file=$(ESP),format=raw,if=ide,index=1 \
	-m 3G -no-reboot $(SERIAL)

## `run` is for actually using the OS, so make it fast: host virtualisation when
## it's available, and no per-interrupt trace log (that alone throttles anything
## interrupt-heavy - a PIT tick + an int 0x80 per frame - to a crawl). The debug
## targets below keep TCG + `-d int` since the gdbstub and the trace want them.
ACCEL := $(shell test -w /dev/kvm && echo '-enable-kvm -cpu host' || echo '-accel tcg')

run: $(ESP) $(HDD) $(OVMF_VARS)
	@echo "(QEMU $(ACCEL))"
	$(QEMU_UEFI) $(ACCEL)

## `make debug` opens a tmux split: QEMU halted with its gdbstub on :1234 in the
## top pane, a debugger attached below. `make ddd` forces DDD, `make gdb` forces
## gdb -tui; `DBG={ddd,gdb}` does the same. NO_TMUX=1 runs just the halted QEMU.
## Inside the debugger, `kheap on` tracks the buddy / slab / kmalloc allocators
## (scripts/kheap.py, auto-sourced by script.gdb).
gdb: DBG := gdb
ddd: DBG := ddd
debug gdb ddd:
	@DBG='$(DBG)' scripts/debug.sh

_debug-qemu: $(ESP) $(HDD) $(OVMF_VARS)
	@echo "(QEMU)"
	$(QEMU_UEFI) -d int,cpu_reset -D qemu_log.txt -S -s
