CC = i686-elf-gcc
AS = nasm
LD = $(CC)
ARCH = TARGET_I686

SRC_DIR 	= ./src
BUILD_DIR 	= ./build
ISO_DIR 	= ./iso
INCLUDE_DIR = ./include
LIBS_DIR	= $(INCLUDE_DIR)/libs

OUTPUT_BINARY = $(BUILD_DIR)/myos.bin
OUTPUT_ISO    = $(BUILD_DIR)/bootable.iso

# Compiler and linker flags
LIBS = -lgcc
CFLAGS = -g -std=gnu99 -ffreestanding -Wall -Wextra -I$(INCLUDE_DIR) -I$(LIBS_DIR) -D$(ARCH)
ASFLAGS = -felf32 -g
LDFLAGS = -T $(SRC_DIR)/linker/linker.ld -ffreestanding -O2 -nostdlib

# Find ALL C, ASM sources
ALL_C_SOURCES := $(shell find $(SRC_DIR) $(INCLUDE_DIR) -type f -name '*.c')
ALL_ASM_SOURCES := $(shell find $(SRC_DIR) -type f -name '*.asm')

# Get the DIR list from all the SOURCES
ALL_C_DIRS := $(sort $(dir $(ALL_C_SOURCES)))
ALL_ASM_DIRS := $(sort $(dir $(ALL_ASM_SOURCES)))

# Get All the OBJ target path based on the sources
ALL_OBJ := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, \
				$(filter-out $(INCLUDE_DIR)/%, $(ALL_C_SOURCES))) \
			$(patsubst $(INCLUDE_DIR)/%.c, $(BUILD_DIR)/%.o, \
				$(filter $(INCLUDE_DIR)/%, $(ALL_C_SOURCES))) \
			$(patsubst $(SRC_DIR)/%.asm, $(BUILD_DIR)/%.o, \
				$(ALL_ASM_SOURCES))

# Main target
all: build $(OUTPUT_ISO)

# Transform the BIN in a ISO to be acceptable for the bootloader
$(OUTPUT_ISO): $(OUTPUT_BINARY)
	cp $(OUTPUT_BINARY) $(ISO_DIR)/boot
	grub-mkrescue -o $(OUTPUT_ISO) $(ISO_DIR)

# Linking all object files into the final binary output
$(OUTPUT_BINARY): $(ALL_OBJ)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

# Rule for assembling the boot assembly file into an object file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile each .c(C file) found on INCLUDE_DIR recursively to .Os in BUILD_DIR
$(BUILD_DIR)/%.o: $(INCLUDE_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile each .asm(ASM) found on SRC_DIR recursively to each file .o in BUILD_DIR
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Create the build DIR and SUBDIRs
build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	@for dir in $(ALL_C_DIRS); do mkdir -p $(BUILD_DIR)/$$dir; done
	@for dir in $(ALL_ASM_DIRS); do mkdir -p $(BUILD_DIR)/$$dir; done

# Clean build dir
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(ISO_DIR)/boot/myos.bin

dev:
	$(MAKE) clean
	$(MAKE)
	$(MAKE) run
debug:
	$(MAKE) clean
	$(MAKE)
	$(MAKE) run-debug

# Target to use QEMU
run-debug:
	qemu-system-i386 -s -S -cdrom $(OUTPUT_ISO) -no-reboot -d int -D qemu_log.txt -m 4G
run:
	qemu-system-i386 -cdrom $(OUTPUT_ISO) -no-reboot -d int -D qemu_log.txt -m 4G

gdb:
	gdb -tui -ex "target remote :1234" -x script.gdb

# Alvo PHONY
.PHONY: all clean run build
