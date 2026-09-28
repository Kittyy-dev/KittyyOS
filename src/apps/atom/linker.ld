ENTRY(atom_test)

SECTIONS
{
    . = 0xFFFFFFFF82000000;

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