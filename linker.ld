__kernel_start = 0x60000;
__kernel_end   = .;

PHDRS {
    all PT_LOAD FLAGS(5);
}

SECTIONS {
    . = 0x0000000000060000;

    .text : {
        *(.text.entry)
        *(.text)
    } :all

    .rodata : {
        *(.rodata)
    } :all

    .data : {
        *(.data)
    } :all

    .bss : {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    } :all

    .stack : {
        __stack_start = .;
        . = . + 0x40000;
        __stack_end = .;
    } :all
}
