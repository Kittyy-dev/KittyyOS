#ifndef BMP_H
#define BMP_H

#include <stdint.h>
#include <stdbool.h>

bool bmp_draw(const uint8_t* data, uint32_t size, int dst_x, int dst_y);

#endif