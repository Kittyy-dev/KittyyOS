#include <stdint.h>
#include <stdio.h>
#include <ramfs.h>

void read_address(int read_start, int read_end) {
    void *read_start_address = (void*)read_start;
    void *read_end_address = (void*)read_end; 

    uintptr_t place = read_start_address - read_end_address;

    void *place_pointer = (void*)place;

    for (int count=read_start; count <= read_end; count++) {
        int value = *((int*)place_pointer); 
    }
}

