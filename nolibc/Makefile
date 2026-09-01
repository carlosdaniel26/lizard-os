include ../Makefile.inc
OBJ := $(patsubst %.c,../$(BUILD)/nolibc/%.c.o,$(wildcard *.c))

all: $(OBJ)

../$(BUILD)/nolibc/%.c.o: %.c
	@mkdir -p "$$(dirname $@)"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@
	@echo "(CC) $<"

-include $(OBJ:.o=.d)
