#include "qe_platform.h"

#include <string.h>

static void append_char(char *buffer, int size, int *length, char value)
{
    if (*length + 1 < size) buffer[*length] = value;
    (*length)++;
}

static void append_text(char *buffer, int size, int *length,
                        const char *text, int limit)
{
    int count;

    count = 0;
    if (text == NULL) text = "(null)";
    while (*text != '\0' && (limit < 0 || count < limit)) {
        append_char(buffer, size, length, *text++);
        count++;
    }
}

static void append_number(char *buffer, int size, int *length, int value)
{
    char digits[16];
    unsigned long number;
    int count;

    if (value < 0) {
        append_char(buffer, size, length, '-');
        number = (unsigned long)(-(value + 1)) + 1;
    } else {
        number = (unsigned long)value;
    }
    count = 0;
    do {
        digits[count++] = (char)('0' + number % 10);
        number /= 10;
    } while (number != 0 && count < (int)sizeof(digits));
    while (count > 0) append_char(buffer, size, length, digits[--count]);
}

int qe_vsnprintf(char *buffer, int size, const char *format, va_list arguments)
{
    int length;
    int precision;

    length = 0;
    while (*format != '\0') {
        if (*format != '%') {
            append_char(buffer, size, &length, *format++);
            continue;
        }
        format++;
        if (*format == '%') {
            append_char(buffer, size, &length, *format++);
            continue;
        }
        precision = -1;
        if (*format == '.') {
            precision = 0;
            format++;
            while (*format >= '0' && *format <= '9') {
                precision = precision * 10 + (*format++ - '0');
            }
        }
        if (*format == 's') {
            append_text(buffer, size, &length, va_arg(arguments, char *),
                        precision);
            format++;
        } else if (*format == 'd') {
            append_number(buffer, size, &length, va_arg(arguments, int));
            format++;
        } else {
            append_char(buffer, size, &length, '%');
            if (*format != '\0') append_char(buffer, size, &length, *format++);
        }
    }
    if (size > 0) buffer[length < size ? length : size - 1] = '\0';
    return length;
}

int qe_snprintf(char *buffer, int size, const char *format, ...)
{
    va_list arguments;
    int length;

    va_start(arguments, format);
    length = qe_vsnprintf(buffer, size, format, arguments);
    va_end(arguments);
    return length;
}
