.PHONY: run debug hdd iso limine-clean

HDD        := hda.img
HDD_SIZE   := 64
HDD_LABEL  := LIZARD
HDD_PART_LBA := 2048
HDD_PART_OFFSET := $(shell expr $(HDD_PART_LBA) \* 512)
ISO       := lizard-os_x86_64.iso
ISO_ROOT  := build/iso_root
LIMINE_DIR := limine
LIMINE_BRANCH := v9.x-binary

## /sbin is usually off a non-root PATH, so resolve these host tools explicitly.
MKFS_FAT  := $(shell command -v mkfs.fat 2>/dev/null || echo /sbin/mkfs.fat)
SFDISK    := $(shell command -v sfdisk 2>/dev/null || echo /sbin/sfdisk)
MCOPY     := $(shell command -v mcopy 2>/dev/null || echo /usr/bin/mcopy)

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
## is where mkfs.fat --offset writes it. mkfs.fat auto-sizes the filesystem to
## (image size - offset); sfdisk auto-sizes the partition the same way, so they
## match. mcopy (mtools) stages files into the partition without a loop mount.

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

## ---- Limine bootloader ------------------------------------------------------
## get-deps only fetches limine.h; the bootloader binaries + host tool that
## build the bootable ISO are pulled here on first use.

$(LIMINE_DIR)/limine:
	rm -rf $(LIMINE_DIR)
	git clone https://github.com/limine-bootloader/limine.git \
		--branch=$(LIMINE_BRANCH) --depth=1 $(LIMINE_DIR)
	$(MAKE) -C $(LIMINE_DIR)

limine-clean:
	rm -rf $(LIMINE_DIR)

## ---- Bootable ISO ---------------------------------------------------------------

iso: $(ISO)

$(ISO): all limine.conf $(LIMINE_DIR)/limine
	@command -v xorriso >/dev/null || { \
		echo "ERROR: xorriso not found - install it with: sudo apt install xorriso"; \
		exit 1; }
	rm -rf $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot/limine $(ISO_ROOT)/EFI/BOOT
	cp $(BIN)/$(OUTPUT) $(ISO_ROOT)/boot/kernel
	cp limine.conf $(ISO_ROOT)/boot/limine/limine.conf
	cp $(LIMINE_DIR)/limine-bios.sys \
	   $(LIMINE_DIR)/limine-bios-cd.bin \
	   $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI $(LIMINE_DIR)/BOOTIA32.EFI $(ISO_ROOT)/EFI/BOOT/
	xorriso -as mkisofs -R -r -J \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISO_ROOT) -o $(ISO)
	$(LIMINE_DIR)/limine bios-install $(ISO)
	rm -rf $(ISO_ROOT)

## ---- QEMU --------------------------------------------------------------------

## Kernel console (COM1) is mirrored to the terminal and to KERNEL_LOG.
KERNEL_LOG := kernel_log.txt
SERIAL     := -chardev stdio,id=com1,logfile=$(KERNEL_LOG) -serial chardev:com1

run: $(ISO) $(HDD)
	@echo "(QEMU)"
	qemu-system-$(ARCH) \
		-M pc \
		-drive file=$(HDD),format=raw,if=ide \
		-cdrom $(ISO) \
		-boot d \
		-m 3G -no-reboot $(SERIAL) -d int,cpu_reset -D qemu_log.txt

debug: $(ISO) $(HDD)
	@echo "(QEMU)"
	qemu-system-$(ARCH) \
		-M pc \
		-drive file=$(HDD),format=raw,if=ide \
		-cdrom $(ISO) \
		-boot d \
		-m 3G -no-reboot $(SERIAL) -d int,cpu_reset -D qemu_log.txt \
		-S -s
