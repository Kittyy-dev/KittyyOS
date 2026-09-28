#pragma once

#include <stdint.h>
#include <stdbool.h>

void mouse_init(void);
void mouse_irq_handler(void);

int mouse_get_x(void);
int mouse_get_y(void);

bool mouse_left(void);
bool mouse_right(void);
bool mouse_middle(void);