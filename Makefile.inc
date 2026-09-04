ARCH := x86_64
BUILD := build
BIN := build/bin
ARCH := x86_64
BUILD := build
BIN := build/bin
CC := gcc
CFLAGS := -g -O0 -pipe -Wall -Wextra -std=gnu11 -nostdinc -ffreestanding -fno-stack-protector -fno-stack-check -fno-PIE -fno-pic -ffunction-sections -fdata-sections -m64 -march=x86-64 -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -no-pie
CPPFLAGS := -I .. -DLIMINE_API_REVISION=3
AS := fasm

# Color Macros
NC      := \033[0m
GREEN   := \033[0;32m
YELLOW  := \033[1;33m
LIGHTY  := \033[1;93m
