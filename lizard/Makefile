include ../Makefile.inc
OBJ := $(patsubst %.c,../$(BUILD)/lizard/%.c.o,$(wildcard *.c)) \
       $(patsubst %.S,../$(BUILD)/lizard/%.S.o,$(wildcard *.S)) \
       $(patsubst %.asm,../$(BUILD)/lizard/%.asm.o,$(wildcard *.asm))

all: $(OBJ)

../$(BUILD)/lizard/%.c.o: %.c
	@mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

../$(BUILD)/lizard/%.S.o: %.S
	@mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

../$(BUILD)/lizard/%.asm.o: %.asm
	@mkdir -p "$$(dirname $@)"
	nasm $(NASMFLAGS) $< -o $@

