#include <stdarg.h>
#include <stddef.h>
#include <limits.h>

#include <stdio.h>
#include <string.h>
#include <kernel/terminal/terminal.h>


bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;

		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
			
		} 
		else if (strsIsEqual(format, "llu", 3)) {
			format+=3;
			uint64_t number = (uint64_t) va_arg(parameters, uint64_t);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			uint32_t size = get_unsigned2string_final_size(number);
			char str[size];
			memset(str, 0, sizeof(str));

			unsigned_to_string((uint64_t)number, str);
			if (!print(str, sizeof(str)))
				return -1;
			written += size;

		}
		else if (*format == 'u') {
			format++;
			unsigned number = (unsigned) va_arg(parameters, unsigned);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			uint32_t size = get_unsigned2string_final_size(number);
			char str[size];
			memset(str, 0, sizeof(str));

			unsigned_to_string((uint64_t)number, str);
			if (!print(str, sizeof(str)))
				return -1;
			written++;

		}
		else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}

int putchar(char character)
{
    terminal_write(&character, sizeof(character));

    return character;
}

int puts(const char*);