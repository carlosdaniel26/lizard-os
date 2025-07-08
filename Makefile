# Nuke built-in rules and variables.
MAKEFLAGS += -rR
.SUFFIXES:

ARCH := x86_64

QEMUDEBUGFLAGS := -S -s
QEMUFLAGS := -m 3G -no-reboot -d int,cpu_reset -D qemu_log.txt

override IMAGE_NAME := lizard-os_$(ARCH)

BUILD := build/$(ARCH)/kernel

CC := gcc
CFLAGS := -g -O2 -pipe
CPPFLAGS :=
LDFLAGS :=
LIBS :=

# Targets

.PHONY: all
all: $(IMAGE_NAME).iso
	@echo "(ISO) $(IMAGE_NAME) generated!!"

.PHONY: run
run: $(IMAGE_NAME).iso
	@echo "(QEMU)"
	@qemu-system-$(ARCH) \
		-M q35 \
		-cdrom $(IMAGE_NAME).iso \
		-boot d \
		$(QEMUFLAGS)

.PHONY: debug
debug: $(IMAGE_NAME).iso
	@echo "(QEMU)"
	@qemu-system-$(ARCH) \
		-M q35 \
		-cdrom $(IMAGE_NAME).iso \
		-boot d \
		$(QEMUFLAGS) \
		$(QEMUDEBUGFLAGS)

ovmf/ovmf-code-$(ARCH).fd:
	mkdir -p ovmf
	curl -Lo $@ https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-$(ARCH).fd

limine/limine:
	rm -rf limine
	git clone https://github.com/limine-bootloader/limine.git --branch=v9.x-binary --depth=1
	$(MAKE) -C limine \
		CC="$(CC)" \
		CFLAGS="$(CFLAGS)" \
		CPPFLAGS="$(CPPFLAGS)" \
		LDFLAGS="$(LDFLAGS)" \
		LIBS="$(LIBS)"

kernel-deps:
	./kernel/get-deps
	touch kernel-deps

.PHONY: kernel
kernel: kernel-deps
	$(MAKE) -C kernel

# Build

$(IMAGE_NAME).iso: limine/limine kernel
	@rm -rf iso_root
	@mkdir -p iso_root/boot
	@cp $(BUILD)/$(ARCH)/kernel iso_root/boot/
	@mkdir -p iso_root/boot/limine
	@cp limine.conf iso_root/boot/limine/
	@mkdir -p iso_root/EFI/BOOT
ifeq ($(ARCH),x86_64)
	@cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	@cp limine/BOOTX64.EFI iso_root/EFI/BOOT/
	@cp limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	@xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso \
		> /dev/null 2>&1
	@./limine/limine bios-install $(IMAGE_NAME).iso \
		> /dev/null 2>&1
endif
	@rm -rf iso_root

$(IMAGE_NAME).hdd: limine/limine kernel
	@rm -f $(IMAGE_NAME).hdd
	@dd if=/dev/zero bs=1M count=0 seek=64 of=$(IMAGE_NAME).hdd

	PATH=$$PATH:/usr/sbin:/sbin sgdisk $(IMAGE_NAME).hdd -n 1:2048 -t 1:ef00 -m 1
	@./limine/limine bios-install $(IMAGE_NAME).hdd

	@mformat -i $(IMAGE_NAME).hdd@@1M
	@mmd -i $(IMAGE_NAME).hdd@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	@mcopy -i $(IMAGE_NAME).hdd@@1M $(BUILD)/$(ARCH)/kernel ::/boot
	@mcopy -i $(IMAGE_NAME).hdd@@1M limine.conf ::/boot/limine

	@mcopy -i $(IMAGE_NAME).hdd@@1M limine/limine-bios.sys ::/boot/limine
	@mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	@mcopy -i $(IMAGE_NAME).hdd@@1M limine/BOOTIA32.EFI ::/EFI/BOOT

.PHONY: clean
clean:
	$(MAKE) -C kernel clean
	@rm -rf iso_root $(IMAGE_NAME).iso $(IMAGE_NAME).hdd

.PHONY: distclean
distclean:
	$(MAKE) -C kernel distclean
	@rm -rf iso_root *.iso *.hdd kernel-deps limine ovmf

gdb:
	gdb -tui -ex "target remote :1234" -x script.gdb