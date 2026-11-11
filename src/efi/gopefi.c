#include <efi.h>
#include <colorsefi.h>
#include <stdint.h>
#include <gopefi.h>
#include <stdarg.h>

void efi_print(EFI_SYSTEM_TABLE *SystemTable, const char *String) {
    CHAR16 buffer[2];

    buffer[1] = 0;

    while (*String) {

        UINT8 code = (UINT8)*String;

        if (code >= 0xF0) {
            SystemTable->ConOut->SetAttribute(SystemTable->ConOut, code & 0x0F);

            String++;
            continue;
        }

        buffer[0] = (CHAR16)*String;

        SystemTable->ConOut->OutputString(
            SystemTable->ConOut,
            buffer
        );

        String++;
    }
}

void efi_clear_screen(EFI_SYSTEM_TABLE *SystemTable) {
    if (SystemTable == 0 || SystemTable->ConOut == 0 || SystemTable->ConOut->ClearScreen == 0) {
        return;
    }

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
}

void efi_set_color(EFI_SYSTEM_TABLE *SystemTable, UINTN foreground, UINTN background) {
    UINTN attribute = foreground | (background << 4);

    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, attribute);
}

void itoa(int value, char* str, int base) {
    char* rc = str;
    char* ptr;
    char* low;

    if(value == 0) {
        *str++ = '0';
        *str = '\0';
        return;
    }

    int sign = value;

    if(sign < 0 && base == 10) {
        value = -value;
    }

    while(value !=0) {
        int rem = value % base;
        *str++ = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value /= base;
    }

    if (sign < 0 && base == 10) {
        *str++ = '-';
    }

    *str = '\0';

    low = rc;
    ptr = str - 1;

    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
}

void u64toa(uint64_t value, char *str, int base) {
    char *rc = str;
    char *ptr;
    char *low;

    if (value == 0) {
        *str++ = '0';
        *str = 0;
        return;
    }

    while (value) {
        uint64_t rem = value % base;
        *str++ = (rem > 9) ? ('a' + rem - 10) : ('0' + rem);
        value /= base;
    }

    *str = 0;

    low = rc;
    ptr = str - 1;

    while (low < ptr) {
        char t = *low;
        *low++ = *ptr;
        *ptr-- = t;
    }
}

void ftoa(double value, char* buffer, int precision) {
    if (value < 0) {
        *buffer++ = '-';
        value = -value;
    }

    uint64_t int_part = (uint64_t)value;
    double frac_part = value - (double)int_part;

    char intbuf[32];
    u64toa(int_part, intbuf, 10);

    int i = 0;

    while (intbuf[i]) {
        *buffer++ = intbuf[i++];
    }

    *buffer++ = '.';

    for (int p = 0; p < precision; p++) {
        frac_part *= 10.0;
        int digit = (int)frac_part;
        *buffer++ = '0' + digit;
        frac_part -= digit;
    }

    *buffer = 0;
}

void efi_printf(EFI_SYSTEM_TABLE *SystemTable, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char out[1024];
    int i = 0;

    while (*fmt && i < 255) {
        if (*fmt == '%') {
            fmt++;

            char buffer[64];

            int zero_pad = 0;
            int width = 0;
            int longlong = 0;

            if (*fmt == '0') {
                zero_pad = 1;
                fmt++;
            }

            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }

            if (*fmt == 'l' && *(fmt + 1) == 'l') {
                longlong = 1;
                fmt +=2;
            }

            switch (*fmt) {
                case 'd': {
                    itoa(va_arg(args, int), buffer, 10);
                    for (int j = 0; buffer[j]; j++)
                        out[i++] = buffer[j];
                    break;
                }

                case 'x': {
                    if (longlong) {
                        u64toa(va_arg(args, uint64_t), buffer, 16);
                    } else {
                        itoa(va_arg(args, int), buffer, 16);
                    }

                    int len = 0;
                    while (buffer[len]) {
                        len++;
                    }

                    while (zero_pad && len < width) {
                        out[i++] = '0';
                        width--;
                    }

                    for (int j = 0; buffer[j]; j++)
                        out[i++] = buffer[j];
                    break;
                }

                case 'p' : {
                    uintptr_t ptr = (uintptr_t)va_arg(args, void*);

                    out[i++] = '0';
                    out[i++] = 'x';

                    #if UINTPTR_MAX == UINT64_MAX
                        u64toa((uintptr_t)ptr, buffer, 16);
                    #else 
                        itoa((int)ptr, buffer, 16);
                    #endif

                    int len = 0;

                    while (buffer[len]) {
                        len++;
                    }

                    int ptr_width = sizeof(void*) * 2;

                    while (len < ptr_width) {
                        out[i++] = '0';
                        len++;
                    }

                    for (int j = 0; buffer[j]; j++) {
                        out[i++] = buffer[j];
                    }

                    break;
                }

                case 's': {
                    char* s = va_arg(args, char*);
                    for (int j = 0; s[j]; j++)
                        out[i++] = s[j];
                    break;
                }

                case 'c': {
                    out[i++] = (char)va_arg(args, int);
                    break;
                }

                case 'u': {
                    unsigned int val = va_arg(args, unsigned int);
                    u64toa((uint64_t)val, buffer, 10);

                    for (int j = 0; buffer[j]; j++) {
                        out[i++] = buffer[j];
                    }
                    break;
                }

                case 'f': {
                    double val = va_arg(args, double);
                    ftoa(val, buffer, 3);

                    for (int j = 0; buffer[j]; j++) {
                        out[i++] = buffer[j];
                    }

                    break;
                }

                default:
                    out[i++] = '%';
                    out[i++] = *fmt;
                    break;
            }

        } else {
            out[i++] = *fmt;
        }

        fmt++;
    }

    out[i] = 0;

    va_end(args);

    efi_print(SystemTable,out);

    // flush(out, sizeof(out));
}

void efi_print_hex(EFI_SYSTEM_TABLE *SystemTable, UINT64 value) {
    char hex[] = "0123456789ABCDEF";
    char buffer[17];

    buffer[16] = '\0';

    for (int i = 15; i >= 0; i--) {
        buffer[i] = hex[value & 0xF];
        value >>= 4;
    }

    efi_print(SystemTable, buffer);
}