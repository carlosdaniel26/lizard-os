# Nuke built-in rules and variables.
MAKEFLAGS += -rR
.SUFFIXES:

# This is the name that our final executable will have.
# Change as needed.
override OUTPUT := kernel

# Target architecture to build for. Default to x86_64.
ARCH := x86_64

# Install prefix; /usr/local is a good, standard default pick.
PREFIX := /usr/local

# Check if the architecture is supported.
ifeq ($(filter $(ARCH),aarch64 loongarch64 riscv64 x86_64),)
	$(error Architecture $(ARCH) not supported)
endif

BUILD := build/$(ARCH)/kernel

# User controllable C compiler command.
CC := gcc

# User controllable C flags.
CFLAGS := -g -O0 -pipe

# User controllable C preprocessor flags. We set none by default.
CPPFLAGS :=

ifeq ($(ARCH),x86_64)
	# User controllable nasm flags.
	NASMFLAGS := -F dwarf -g
endif

# User controllable linker flags. We set none by default.
LDFLAGS :=

# Ensure the dependencies have been obtained.
ifneq ($(shell ( test '$(MAKECMDGOALS)' = clean || test '$(MAKECMDGOALS)' = distclean ); echo $$?),0)
	ifeq ($(shell ( ! test -f src/limine.h ); echo $$?),0)
        	$(error Please run the ./get-deps script first)
	endif
endif

# Check if CC is Clang.
override CC_IS_CLANG := $(shell ! $(CC) --version 2>/dev/null | grep 'clang' >/dev/null 2>&1; echo $$?)

# Internal C flags that should not be changed by the user.
override CFLAGS += \
	-Wall \
	-Wextra \
	-std=gnu11 \
	-nostdinc \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-PIC \
	-ffunction-sections \
	-fdata-sections

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
	-I lizard \
	-I nolibc \
	-I runtime \
	-I . \
	$(CPPFLAGS) \
	-DLIMINE_API_REVISION=3 \
	-MMD \
	-MP

ifeq ($(ARCH),x86_64)
	# Internal nasm flags that should not be changed by the user.
	override NASMFLAGS += \
		-Wall
endif

# Architecture specific internal flags.
ifeq ($(ARCH),x86_64)
	ifeq ($(CC_IS_CLANG),1)
		override CC += \
			-target x86_64-unknown-none
	endif
	override CFLAGS += \
		-m64 \
		-march=x86-64 \
		-mno-80387 \
		-mno-mmx \
		-mno-sse \
		-mno-sse2 \
		-mno-red-zone \
		-mcmodel=kernel
	override LDFLAGS += \
		-Wl,-m,elf_x86_64
	override NASMFLAGS += \
		-f elf64
endif

# Internal linker flags that should not be changed by the user.
override LDFLAGS += \
	-Wl,--build-id=none \
	-nostdlib \
	-static \
	-z max-page-size=0x1000 \
	-Wl,--gc-sections \
	-T linker-$(ARCH).ld

.PHONY: lizard nolibc runtime
lizard:
	$(MAKE) -C lizard
nolibc:
	$(MAKE) -C nolibc
runtime:
	$(MAKE) -C runtime

OBJ = $(shell find build/$(ARCH)/kernel -name "*.o")

# Default target. This must come first, before header dependencies.
.PHONY: all
all: lizard nolibc runtime bin/$(OUTPUT)

# Link rules for the final executable.
$(BIN)/$(OUTPUT): lizard nolibc runtime linker-$(ARCH).ld
	@mkdir -p "$(BIN)"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(shell find $(BUILD) -name "*.o") -o $@

# Compilation rules for *.c files.
$(BUILD)/$(ARCH)/%.c.o: %.c Makefile
	@mkdir -p "$$(dirname $@)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	@echo "(CC) $<"

# Compilation rules for *.S files.
$(BUILD)/$(ARCH)/%.S.o: %.S Makefile
	@mkdir -p "$$(dirname $@)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

ifeq ($(ARCH),x86_64)
# Compilation rules for *.asm (nasm) files.
$(BUILD)/$(ARCH)/%.asm.o: %.asm Makefile
	@mkdir -p "$$(dirname $@)"
	@nasm $(NASMFLAGS) $< -o $@
	@echo "(AS) $<"
endif

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf build obj-$(ARCH)

# Include emulation targets
include makefiles/emulation.mk

# Remove everything built and generated including downloaded dependencies.
.PHONY: clean distclean
clean:
	rm -rf build obj-$(ARCH)

distclean: clean
	rm -rf lizard/limine.h

# Install the final built executable to its final on-root location.
.PHONY: install
install: all
	install -d "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)"
	install -m 644 $(BUILD)/$(OUTPUT) "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)/$(OUTPUT)-$(ARCH)"

# Try to undo whatever the "install" target did.
.PHONY: uninstall
uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)/$(OUTPUT)-$(ARCH)"
	-rmdir "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)"

gdb:
	gdb -tui -ex "target remote :1234" -x script.gdb

