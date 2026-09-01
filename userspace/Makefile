.PHONY: all clean

## Freestanding userspace test program. Needs the repo root and nolibc on the
## include path because syscall.h pulls <lizard/*.h> and <nolibc/types.h>.
CFLAGS := -I.. -I../nolibc -ffreestanding -nostdlib -m64 -fno-pie
OBJS   := crt0.o hello.o syscall.o

all: hello

hello: $(OBJS)
	ld -Ttext 0x0 -e _start -o $@ $(OBJS)

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f hello $(OBJS)
