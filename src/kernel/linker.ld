KERNEL_VMA = 0xFFFFFFFF80000000;
KERNEL_LMA = 0x0000000000100000;

__kernel_start = KERNEL_VMA;
__kernel_phys_start = LOADADDR(.text);
__kernel_phys_end = LOADADDR(.bss) + SIZEOF(.bss);

PHDRS
{
    all PT_LOAD FLAGS(7); /* R + W + X */
}

SECTIONS
{
    . = KERNEL_VMA;

    .text : AT(KERNEL_LMA)
    {
        *(.text.entry)
        *(.text*)
    } :all

    .rodata : AT(KERNEL_LMA + (ADDR(.rodata) - KERNEL_VMA))
    {
        *(.rodata*)
    } :all

    .data : AT(KERNEL_LMA + (ADDR(.data) - KERNEL_VMA))
    {
        *(.data*)
    } :all

    .bss : AT(KERNEL_LMA + (ADDR(.bss) - KERNEL_VMA))
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } :all

    __kernel_end = .;
}