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

## UEFI firmware (OVMF). VARS is copied to a writable per-tree file.
OVMF_CODE  := /usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS  := ovmf_vars.fd
OVMF_VARS_SRC := /usr/share/OVMF/OVMF_VARS_4M.fd

## /sbin is usually off a non-root PATH, so resolve these host tools explicitly.
MKFS_FAT  := $(shell command -v mkfs.fat 2>/dev/null || echo /sbin/mkfs.fat)
SFDISK    := $(shell command -v sfdisk 2>/dev/null || echo /sbin/sfdisk)
MCOPY     := $(shell command -v mcopy 2>/dev/null || echo /usr/bin/mcopy)
MMD       := $(shell command -v mmd 2>/dev/null || echo /usr/bin/mmd)

## Userspace programs staged into the FAT16 root. main.c does
## vfs_read_all("/hello") + load_elf(), so /hello must be an ET_EXEC ELF.
HELLO := userspace/hello

$(HELLO): userspace/hello.c userspace/crt0.c userspace/syscall.c userspace/Makefile
	$(MAKE) -C userspace

## ---- Root disk image (MBR + FAT16 partition) --------------------------------
## The kernel boots with root=ata0p0, so the image needs a real MBR partition
## table. Layout: one bootable FAT16 (type 0x0e) partition starting at LBA 2048,
## spanning the rest of the disk. lizard/fat16.c reads the BPB from LBA 0 of the
## *partition* block device (blk_dev_part_read adds the partition start), which
## is where mkfs.fat --offset writes it.

$(HDD): $(HELLO)
	@for pair in "$(MKFS_FAT):dosfstools" "$(SFDISK):fdisk" "$(MCOPY):mtools"; do \
		bin=$${pair%:*}; pkg=$${pair##*:}; \
		command -v $$bin >/dev/null 2>&1 || { \
			echo "ERROR: $$(basename $$bin) not found - install it with: sudo apt install $$pkg"; \
			exit 1; }; \
	done
	dd if=/dev/zero of=$@ bs=1M count=$(HDD_SIZE) status=none
	printf 'label: dos\nstart=%s, type=0e, bootable\n' $(HDD_PART_LBA) | $(SFDISK) -q $@
	$(MKFS_FAT) -F 16 -s 4 -S 512 -r 512 -R 1 --offset $(HDD_PART_LBA) -n $(HDD_LABEL) $@
	MTOOLS_SKIP_CHECK=1 $(MCOPY) -i $@@@$(HDD_PART_OFFSET) $(HELLO) ::/HELLO

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
	-m 3G -no-reboot $(SERIAL) -d int,cpu_reset -D qemu_log.txt

run: $(ESP) $(HDD) $(OVMF_VARS)
	@echo "(QEMU)"
	$(QEMU_UEFI)

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
	$(QEMU_UEFI) -S -s
