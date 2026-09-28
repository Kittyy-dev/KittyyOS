#include <stdint.h>
#include <string.h>
#include <kcolors.h>
#include <vbe.h>

int kprint_is_color_code(const char* str) {
    if (str[0] != '\033') {
        return 0;
    }

    if (str[1] != '[') {
        return 0;
    }

    if (str[2] != '0' && str[2] != '1') {
        return 0;
    }

    if (str[3] != ';') {
        return 0;
    }

    if (str[4] != '3') {
        return 0;
    }

    if (str[5] < '0' || str[5] > '7') {
        return 0;
    }

    if (str[6] != 'm') {
        return 0;
    }

    return 1;
}

uint8_t kprint_ansi_to_vbe(const char* str) {
    int color = str[5] - '0';

    if (str[2] == '0') {
        return 0x01 + color;
    }

    return 0x11 + color;
}

void kprint_vbe(const char* str) {
    char buffer[KPRINT_BUFFER_SIZE];

    int i = 0;

    while (*str && i < KPRINT_BUFFER_SIZE - 1) {
        if (kprint_is_color_code(str)) {
            uint8_t vbe_color = kprint_ansi_to_vbe(str);

            buffer[i++] = vbe_color;

            str += 7;

            continue;
        }

        if (str[0] == '\033' && str[1] == '[' && str[2] == '0' && str[3] == 'm') {
            buffer[i++] = 0x08;

            str += 4;

            continue;
        }

        buffer[i++] = *str++;
    }

    buffer[i] = '\0';

    vbe_print(buffer);
}