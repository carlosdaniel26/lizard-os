MAKEFLAGS += --no-print-directory

CC = i686-elf-gcc
AS = nasm
LD = $(CC)
ARCH = TARGET_I686

SRC_DIR 	= ./src
BUILD_DIR 	= ./build
ISO_DIR 	= ./iso
INCLUDE_DIR = ./include
LIBS_DIR	= $(INCLUDE_DIR)/libs

OUTPUT_BINARY = $(BUILD_DIR)/lizard-os.bin
OUTPUT_ISO    = $(BUILD_DIR)/lizard-os.iso

# Compiler and linker flags
LIBS = -lgcc
CFLAGS = -std=gnu99 -ffreestanding -Wall -Wextra -I$(INCLUDE_DIR) -I$(LIBS_DIR) -D$(ARCH) -g
ASFLAGS = -felf32 -g
LDFLAGS = -T $(SRC_DIR)/linker/linker.ld -ffreestanding -O2 -nostdlib -g
QEMUFLAGS = -no-reboot -d int,cpu_reset -D qemu_log.txt -cdrom $(OUTPUT_ISO) -m 4G -M smm=off -rtc base=localtime
KVMFLAGS = -enable-kvm -cpu host 

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
	@echo "(OUTPUT_ISO) Creating ISO"
	@cp $(OUTPUT_BINARY) $(ISO_DIR)/boot
	@grub-mkrescue -o $(OUTPUT_ISO) $(ISO_DIR) >/dev/null 2>&1


# Linking all object files into the final binary output
$(OUTPUT_BINARY): $(ALL_OBJ)
	@echo "(LD) Linking Objects"
	@$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

# Rule for assembling the boot assembly file into an object file
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "(CC) $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile each .c(C file) found on INCLUDE_DIR recursively to .Os in BUILD_DIR
$(BUILD_DIR)/%.o: $(INCLUDE_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile each .asm(ASM) found on SRC_DIR recursively to each file .o in BUILD_DIR
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@echo "(AS) $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@
	

# Create the build DIR and SUBDIRs
build:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub
	@for dir in $(ALL_C_DIRS); do mkdir -p $(BUILD_DIR)/$$dir; done
	@for dir in $(ALL_ASM_DIRS); do mkdir -p $(BUILD_DIR)/$$dir; done

# Clean build dir
clean:
	@rm -rf $(BUILD_DIR)
	@rm -rf $(ISO_DIR)/boot/myos.bin

dev:
	@$(MAKE) clean
	@$(MAKE)
	@$(MAKE) run
qemu-debug:
	@$(MAKE) clean
	@$(MAKE)
	@$(MAKE) run-debug

debug:
	tmux new-session -d -s qemu_session
	tmux new-window -t qemu_session:1 'make qemu-debug'
	tmux new-window -t qemu_session:2 'make gdb; tmux kill-session -t qemu_session'
	tmux kill-window -t qemu_session:0  # Close window 0 explicitly
	tmux attach-session -t qemu_session


# Real Hardawre
rw:
	sudo cp $(OUTPUT_BINARY) /boot/


# Target to use QEMU
run-debug:
	@echo "(QEMU) Running in debug mode"
	@qemu-system-i386 -s -S $(QEMUFLAGS)
run:
	@echo "(QEMU) Running in normal mode"
	@qemu-system-i386 $(QEMUFLAGS) $(KVMFLAGS) 

gdb:
	@gdb -tui -ex "target remote :1234" -x script.gdb

# Alvo PHONY
.PHONY: all clean run build
