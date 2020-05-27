#include <printf.h>
#include <screen.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

char formats_l[] = "xdspcXoi%";
char flags_l[]   = "0-+ #";

int printf(const char *format, ...) {
    char buf[40];

    va_list ap;
    va_start(ap, format);

    int parse_format = 1;
    int written      = 0;

    while (*format != '\0') {
        if (*format == '%' && parse_format) {

            // Check for valid format specifier
            uint32_t i, j;
            for (i = 1; format[i] != '\0'; i++) {
                for (j = 0; formats_l[j] != '\0'; j++) {
                    if (format[i] == formats_l[j])
                        break;
                }
                // A format specifier was found
                if (formats_l[j] != '\0')
                    break;
            }

            char format_c = format[i];

            // No format specifier found
            if (format_c == '\0') {
                parse_format = 0;
                continue;
            }

            // Flags bitfield
            // "0-+ #"
            // Bit 1: 0         Left-pad number with 0s by width
            // Bit 2: -         Left-justify by width
            // Bit 3: +         Force sign for positive numbers
            // Bit 4: <space>   If no sign, put a space
            // Bit 5: #         Precede xXo value with 0x, 0X or 0 (currently broken)
            uint8_t flags = 0;
            int     width = 0;

            // Flags or width present
            // TODO Add length prefix parsing
            if (i > 1) {

                // Which flag index to check out next
                // Used by the width checking block to skip the flags
                uint32_t f_idx = 1;

                // Check all flags
                for (; f_idx < i; f_idx++) {
                    uint32_t k;
                    for (k = 0; flags_l[k] != '\0'; k++) {
                        if (format[f_idx] == flags_l[k])
                            break;
                    }

                    // There is a flag
                    if (flags_l[k] != '\0') {
                        flags |= 1 << k;
                    }
                    // Probably a width specifier and we should break
                    else {
                        break;
                    }
                }

                // Specifies the end of the width specifier
                uint32_t wid_idx = f_idx;

                // Easy route: the width is an argument
                if (format[wid_idx] == '*') {
                    width = va_arg(ap, uint32_t);
                    wid_idx++;
                }
                // Not so easy route: need to convert the number to an uint
                else {
                    for (; format[wid_idx] >= '0' && format[wid_idx] <= '9'; wid_idx++) {
                        width *= 10;
                        width += format[wid_idx] - '0';
                    }
                }
            }

            char *  str;
            uint8_t base;

            // Get base needed for number conversion
            switch (format_c) {
                case 'd':
                case 'i':
                    base = 10;
                    break;
                case 'X':
                case 'x':
                case 'p':
                    base = 16;
                    break;
                case 'o':
                    base = 8;
                    break;
                case 'b':
                    base = 2;
                    break;
                default:
                    base = 0;
                    break;
            }

            // Convert the numbers into a string
            // or get a string from va_arg
            // or pass a character from a tiny buffer
            switch (format_c) {
                case 'd':
                case 'i':
                case 'x':
                case 'X':
                case 'o':
                case 'p':
                case 'b':
                    str = itoa(va_arg(ap, uint32_t), &buf[3], base);
                    break;
                case 's':
                    str = va_arg(ap, char *);
                    break;
                case 'c':
                    buf[0] = (char) va_arg(ap, int);
                    buf[1] = '\0';
                    str    = buf;
                    break;
                case '%':
                    buf[0] = '%';
                    str    = buf;
                    break;
                default:
                    str = "?";
            }

            // Add base prefix for some formats and flag
            if (format_c == 'p' || (flags & 16) == 16) {
                switch (format_c) {
                    case 'p':
                    case 'x':
                    case 'X':
                        buf[1] = '0';
                        buf[2] = 'x';
                        str    = &buf[1];
                        break;
                    case 'o':
                        buf[2] = '0';
                        str    = &buf[2];
                        break;
                    default:
                        break;
                }
            }

            // Upcase on X
            if (format_c == 'X') {
                for (uint32_t i = 0; str[i] != '\0'; i++) {
                    str[i] = toupper(str[i]);
                }
            }

            // Left justify the string
            if ((flags & 2) == 2) {
                do {
                    putchar(*str++);
                    written++;
                } while (*str != '\0');
            }

            // Put `width` bytes of padding
            if (width > 0) {
                width -= strlen(str);
            }
            if (width < 0) {
                width = 0;
            }

            // Put padding
            written += width;
            for (; width > 0; width--) {
                // Use 0s
                if ((flags & 1) == 1) {
                    putchar('0');
                } else {
                    putchar(' ');
                }
            }

            // Right justify the string
            if ((flags & 2) == 0) {
                do {
                    putchar(*str++);
                    written++;
                } while (*str != '\0');
            }

            // On to the last character in the format specifier
            format += i;
        } else {
            if (!parse_format)
                parse_format = 1;
            putchar(*format);
            written++;
        }
        format++;
    }

    va_end(ap);

    return written;
}