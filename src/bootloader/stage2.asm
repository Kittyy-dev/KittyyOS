DEFAULT ABS

[org 0x0000]
[bits 16]

start:
    cli
    mov ax, 0x0800
    mov ds, ax
    mov ss, ax
    mov sp, 0x9000

    mov ax, 0x0800
    mov es, ax

    mov si, msg 
.print_msg:
    lodsb
    test al, al
    jz .msg_done
    mov ah, 0x0E
    int 0x10
    jmp .print_msg

.msg_done:
    mov ax, 79
    add ax, [partition_offset]
    mov cx, 32
    call read_sectors_to_buffer

    mov di, buffer
.search_initrd:
    cmp byte es:[di + 11], 0x0F
    je .next_initrd

    mov ax, 0x0800
    mov ds, ax
    mov si, filename_initrd
    mov cx, 11
    push di
    repe cmpsb
    pop di
    je .found_initrd

.next_initrd:
    add di, 32
    cmp di, buffer + 16384
    jb .search_initrd
    jmp fail

.found_initrd:
    mov ax, es:[di + 26]
    mov [initrd_start_lba], ax
    mov cx, es:[di + 28]
    mov [initrd_size_bytes], cx

    mov di, 0x0000
    mov ax, 0x2000
    mov es, ax
    mov bx, 0x0000
    mov dx, [initrd_start_lba]
    mov ecx, [initrd_size_bytes]
    call load_file
    jc .fail_initrd

.fail_initrd:
    mov ah, 0x0E
    mov al, 'I'
    int 0x10

    mov di, buffer
.search_kernel:
    cmp byte es:[di + 11], 0x0F
    je .next_kernel

    mov ax, 0x0800
    mov ds, ax
    mov si, filename_kernel
    mov cx, 11
    push di
    repe cmpsb
    pop di
    je .found_kernel

.next_kernel:
    add di, 32
    cmp di, buffer + 16384
    jb .search_kernel
    jmp fail

.found_kernel:
    mov ax, es:[di + 26]
    mov dx, ax
    mov cx, es:[di + 28]
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000
    movzx ecx, cx 
    push cx
    push ecx
    call load_file
    mov ah, 0x0E
    mov al, 'O'
    int 0x10
    jc .fail_kernel

    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

.fail_kernel:
    mov ah, 0x0E
    mov al, 'K'
    int 0x10

fail:
    mov ah, 0x0E
    mov al, 'F'
    int 0x10

read_sectors_to_buffer:
    push ax
    push cx
    mov bx, buffer
.next_root_sector:
    push ax
    mov al, 1
    push bx
    call lba_to_chs
    pop bx
    mov ah, 0x02
    int 0x13
    jc fail
    pop ax
    inc ax
    add bx, 512
    loop .next_root_sector
    pop cx
    pop ax
    ret

load_file:
    push ecx
    mov si, bx
.load_next:
    mov ax, dx
    push si
    call lba_to_chs
    pop si
    mov ah, 0x02
    mov al, 1
    mov bx, si
    int 0x13
    jc fail
    inc dx
    add si, 512
    sub ecx, 512
    cmp ecx, 0
    ja .load_next
    pop ecx
    ret

lba_to_chs:
    push ax
    push dx

    xor dx, dx
    div word [sectors_per_track]
    mov bl, dl
    inc bl

    xor dx, dx
    div word [heads_per_cylinder]
    mov dh, dl
    mov cx, ax

    mov cl, bl
    mov ax, cx
    mov ch, al
    mov al, ah
    and al, 0x03
    shl al, 6
    and cl, 0x3F
    or  cl, al

    pop dx
    pop ax
    ret

sectors_per_track:   dw 18
heads_per_cylinder:  dw 2

[bits 32]
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    mov esi, 0x00010000
    mov eax, dword [esi]
    cmp eax, 0x464C457F
    jne kernel_fail_pm

    mov eax, cr4
    or eax, 0x10
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000100
    wrmsr

    mov eax, pml4_table
    add eax, STAGE2_BASE
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    jmp 0x18:long_mode_start

kernel_fail_pm:
    mov ah, 0x0E
    mov al, 'F'
    int 0x10

[bits 64]
long_mode_start:   

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax                                                      
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000

    

    mov rsi, 0x200000
    mov rdx, [initrd_start_lba]
    mov rdi, [initrd_size_bytes]

    mov rdi, 0xB8000
    mov word [rdi], 0x0F54


    ; jmp 0x100000

.hang:
    hlt
    jmp .hang

STAGE2_BASE equ 0x00008000
boot_drive:         db 0
msg:                db "2025 - 2026 Copyright (C) Kittyy   ||   Loading kernel and initrd", 0x0D, 0x0A, 0x0D, 0x0A, 0
filename_kernel:    db 'KERNEL BIN'
filename_initrd:    db 'INITRD IMG'

buffer:             times 16384 db 0

initrd_start_lba:   dd 0
initrd_size_bytes:  dd 0
partition_offset:   dd 0

gdt_start:
    dq 0x0000000000000000         

    ; 0x08: 32-bit Code Segment (für protected_mode)
    dq 0x00CF9A000000FFFF

    ; 0x10: 32-bit Data Segment
    dq 0x00CF92000000FFFF

    ; 0x18: 64-bit Code Segment (L=1, D=0)
    dq 0x00209A0000000000

    ; 0x20: 64-bit Data Segment (kann schlicht sein)
    dq 0x0000920000000000
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start 


align 4096
pml4_table:
    dq pdpt_table + STAGE2_BASE + 3
    times 511 dq 0

pdpt_table:
    dq pd_table + STAGE2_BASE + 3
    times 511 dq 0

pd_table:
    dq pt_table + STAGE2_BASE + 3
    times 511 dq 0


pt_table:
%assign phys 0
%rep 512
    dq phys + 0x83
    %assign phys phys + 0x1000
%endrep
