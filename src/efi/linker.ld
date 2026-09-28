OUTPUT_FORMAT("pei-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(efi_main)

SECTIONS
{
    . = 0;

    .text : ALIGN(4096)
    {
        *(.text*)
    }

    .data : ALIGN(4096)
    {
        *(.data*)
    }

    .rodata : ALIGN(4096)
    {
        *(.rodata*)
    }

    .bss : ALIGN(4096)
    {
        *(.bss*)
        *(COMMON)
    }

    .reloc : ALIGN(4096)
    {
        *(.reloc)
    }
}