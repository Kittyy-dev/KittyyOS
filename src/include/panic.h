#include <isr.h>

#pragma once

void panic(const char* msg);
void panic_ex(const char* msg, uint64_t vector, uint64_t error_code, struct isr_frame* f);