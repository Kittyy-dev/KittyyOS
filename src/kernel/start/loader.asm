%define OFFSET 0x20000
%define PML4_PHYS (OFFSET + pml4)
%define PDPT_PHYS (OFFSET + pdpt)
%define PD_PHYS (OFFSET + pd)
%define LONG_MODE_PHYS (OFFSET + long_mode_entry)

struc IDTEntry64
    .offset_low: resw 1
    .selector: resw 1
    .ist: resb 1
    .type_attr: resb 1
    .offset_mid: resw 1
    .offset_high: resd 1
    .zero: resd 1
endstruc

; %macro set_idt_entry 2
;     lea rax, [idt64 + %1*16]

; %endmacro

org 0

; [bits 64]

extern entry

; section .bss
; e820_buffer:
;     resb 32 * 24

; e820_entries:
;     resw 1

[bits 16]

section .text16

global _start

_start:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov sp, 0xFFFE

    mov ah, 0x0E
    mov al, 'K'
    int 0x10

    mov ax, 0x7000
    mov es, ax
    xor bx, bx

    mov dx, 2118        ; Start-LBA

    mov cx, 127
    call load_sectors_lba

    add bx, 127 * 512

    add dx, 127
    mov cx, 73
    call load_sectors_lba

    ; mov bx, 0
    ; call check_elf_header
    ; cmp ax, 0
    ; jne bad_kernel

    mov ah, 0x0E
    mov al, '!'
    int 0x10

    cli

    mov ax, cs
    movzx eax, ax
    shl eax, 4

    add eax, gdt_start
    mov [gdt_descriptor+2], eax

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:pm_entry

stub_base: dd 0

load_sectors_lba:
    push si
    push di
    push ds

    mov si, dap

    mov byte [si], 0x10
    mov byte [si+1], 0
    mov word [si+2], cx
    mov word [si+4], bx
    mov word [si+6], es

    mov word [si+8], dx
    mov word [si+10], 0

    mov word [si+12], 0
    mov word [si+14], 0

    mov ah, 0x42
    mov dl, 0x80
    int 0x13

    jc disk_error

    pop ds
    pop di
    pop si
    ret

check_elf_header:
    push ds
    push si

    mov si, bx
    mov ax, es
    mov ds, ax

    cmp byte [si], 0x7F
    jne .fail

    cmp byte [si+1], 'E'
    jne .fail

    cmp byte [si+2], 'L'
    jne .fail

    cmp byte [si+3], 'F'

    pop si
    pop ds
    xor ax, ax
    ret

.fail:
    pop si
    pop ds
    mov ax, 1

    mov ah, 0x0E
    mov al, 'F'
    int 0x10

bad_kernel:
    cli
    hlt
    jmp $

disk_error:
    cli
    mov ah, 0x0E
    mov al, 'E'
    hlt
    jmp $

.hang:
    jmp .hang

[bits 32]

section .text32

pm_entry:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    ; hlt

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x9FC00

    mov eax, gdt64_start
    add eax, OFFSET
    mov [gdt64_decs+2], eax

    lgdt [gdt64_decs]

    mov esp, 0x9FC00

    ; Paging start

    ; PD 1GB Identity(First 2MB)
    mov eax, 0x00000083
    mov edi, PD_PHYS
    mov [edi], eax
    mov dword [edi+4], 0

    ; PDPT[0] -> PD
    mov eax, PD_PHYS
    or eax, 0x3
    mov edi, PDPT_PHYS
    mov [edi], eax
    mov dword [edi+4], 0

    ; PML4[0] -> PDPT
    mov eax, PDPT_PHYS
    or eax, 0x3
    mov edi, PML4_PHYS
    mov [edi], eax
    mov dword [edi+4], 0

    mov eax, PML4_PHYS
    mov cr3, eax

    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or  eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    mov ebx, LONG_MODE_PHYS

    jmp 0x08:LONG_MODE_PHYS

.hang:
    jmp .hang

empty_idt:
    dw 0
    dd 0

[bits 64]

section .text64

KERNEL_PHYS_BASE equ 0x00060000

ELF_BASE equ 0x00070000
KERNEL_DST  equ 0x00060000
KERNEL_OFF  equ 0x00001000
KERNEL_SIZE equ 0x00050000
KERNEL_ENTRY equ 0x00060000

global long_mode_entry

dummy_qword: dq 0

dummy_isr:
    cli

long_mode_entry:
    cli

    mov rcx, 0

    mov rsi, ELF_BASE
    add rsi, KERNEL_OFF

    mov rdi, KERNEL_DST

    mov rcx, KERNEL_SIZE

    rep movsb

    mov rax, KERNEL_ENTRY

    jmp rax

.hang64:
    jmp .hang64

isr0:
    cli
    hlt
    jmp isr0

; times 375 db 0

[bits 16]

section .text16

dap:
    times 16 db 0

section .gdt

gdt_start: ; gdt32
gdt_null:       dq 0


gdt_code32:
    dw 0xFFFF
    dw 0x0000
    db 0x02
    db 10011010b
    db 11001111b
    db 0x00

gdt_data32:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_code64:     dq 0x00AF9A000000FFFF

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt64_start: ; gdt 64
    dq 0

gdt64_code64:     dq 0x00209A0000000000

gdt64_data64: dq 0x0000930000000000

gdt64_end:

gdt64_decs:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start + OFFSET

section .idt

global isr_default

isr_default:
    cli

.hang:
    hlt
    jmp .hang

align 8

idt_table:
    times 256 dq 0

idt_table_end:

idt_descriptor:
    dw idt_table_end - idt_table - 1
    dd idt_table

make_idt_entry:
    push edx
    mov edx, idt_table
    lea edx, idt_table

    mov word [edx], ax
    mov word [edx+2], 0x0008
    mov byte [edx+4], 0
    mov byte [edx+5], 0x8E
    shr eax, 16
    mov word [edx+6], ax
    pop edx
    ret

section .pgtables align=4096

pml4:
    times 512 dq 0

pdpt:
    times 512 dq 0

pd: times 512 dq 0

; times 730 db 0

section .bss

align 16

idt64: resb 256 * 16

section .data

global long_mode_offset

long_mode_offset:
    dq long_mode_entry - $$

jmp_target:
    dw 0
    dd 0

message: db "LONG MODE (64BIT INCSTRUCTIONS AVIABLE!)", 0

idtr64:
    dw 256*16 - 1
    dq idt64

times 447 db 0
