.PHONY: all

all: hello

hello: hello.c crt0.c syscall.c
	gcc -I../kernel/src -ffreestanding -nostdlib -m64 -fno-pie -c hello.c -o hello.o
	gcc -I../kernel/src -ffreestanding -nostdlib -m64 -fno-pie -c crt0.c -o crt0.o
	gcc -I../kernel/src -ffreestanding -nostdlib -m64 -fno-pie -c syscall.c -o syscall.o
	ld -Ttext 0x0 -o hello crt0.o hello.o syscall.o

mount:
	sudo mount -o loop ../hda.img /mnt/lizard
	sudo cp hello /mnt/lizard/hello
	sudo umount /mnt/lizard

