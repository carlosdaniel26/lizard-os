ARCH := x86_64
BUILD := build
BIN := build/bin
CC := gcc
CFLAGS := -g -O0 -pipe -Wall -Wextra -std=gnu11 -nostdinc -ffreestanding -fno-stack-protector -fno-stack-check -fno-PIC -ffunction-sections -fdata-sections -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel
CPPFLAGS := -I .. -I ../lizard -I ../nolibc -I ../runtime -DLIMINE_API_REVISION=3
NASMFLAGS := -F dwarf -g -Wall -f elf64

