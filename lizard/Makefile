include ../Makefile.inc
OBJ := $(patsubst %.c,../$(BUILD)/lizard/%.c.o,$(wildcard *.c)) \
       $(patsubst %.S,../$(BUILD)/lizard/%.S.o,$(wildcard *.S)) \
       $(patsubst %.asm,../$(BUILD)/lizard/%.asm.o,$(wildcard *.asm))

DEPFLAGS := -MMD -MP

all: $(OBJ)

../$(BUILD)/lizard/%.c.o: %.c
	@mkdir -p "$$(dirname $@)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@
	@echo "(CC) $<"

../$(BUILD)/lizard/%.S.o: %.S
	@mkdir -p "$$(dirname $@)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@
	@echo "(CC) $<"

../$(BUILD)/lizard/%.asm.o: %.asm
	@mkdir -p "$$(dirname $@)"
	@$(AS) $< $@ >/dev/null
	@echo "(AS) $<"

-include $(OBJ:.o=.d)
