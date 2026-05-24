include ../Makefile.inc
OBJ := $(patsubst %.c,../$(BUILD)/nolibc/%.c.o,$(wildcard *.c))

all: $(OBJ)

../$(BUILD)/nolibc/%.c.o: %.c
	@mkdir -p "$$(dirname $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
