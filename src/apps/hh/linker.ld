ENTRY(hh_entry)

SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text :
    {
        *(.text.entry)
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
        *(COMMON)
    }
}