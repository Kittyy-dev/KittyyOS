ENTRY(module_entry)

SECTIONS
{
    . = 0xFFFFFFFF81000000;

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