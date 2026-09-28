/* link.ld */
OUTPUT_FORMAT(binary)
ENTRY(start_stage2)
SECTIONS {
  . = 0x8000;
  .text : { *(.text*) }
  .data : { *(.data*) }
  /DISCARD/ : { *(.bss*) *(.comment*) }
}
