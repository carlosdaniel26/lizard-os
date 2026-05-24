include ../Makefile.inc
OBJ := $(patsubst %.c,../$(BUILD)/runtime/%.c.o,$(wildcard *.c))

all: $(OBJ)

../$(BUILD)/runtime/%.c.o: %.c
	@mkdir -p "$$(dirname $@)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	@echo "(CC) $<"
