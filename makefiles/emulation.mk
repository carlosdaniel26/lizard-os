.PHONY: run debug
run: all
	@echo "(QEMU)"
	qemu-system-$(ARCH) \
		-M pc \
		-drive file=hda.img,format=raw,if=ide \
		-cdrom lizard-os_x86_64.iso \
		-boot d \
		-m 3G -no-reboot -d int,cpu_reset -D qemu_log.txt

debug: all
	@echo "(QEMU)"
	qemu-system-$(ARCH) \
		-M pc \
		-drive file=hda.img,format=raw,if=ide \
		-cdrom lizard-os_x86_64.iso \
		-boot d \
		-m 3G -no-reboot -d int,cpu_reset -D qemu_log.txt \
		-S -s
