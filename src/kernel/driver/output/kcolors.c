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