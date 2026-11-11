DEFAULT ABS

STAGE2_BASE equ 0x8000

BOOT_INFO_ADDR equ 0x3000

BOOT_VBE_CTRL equ BOOT_INFO_ADDR + 0x20
BOOT_VBE_MODE equ BOOT_INFO_ADDR + 0x220
BOOT_VBE_OEM  equ BOOT_INFO_ADDR + 0x320
BOOT_E820     equ BOOT_INFO_ADDR + 0x420

HIGH_PDPT_ADDR equ 0x73000
HIGH_PD_ADDR   equ 0x74000

MAX_E820_ENTRIES equ 64
E820_ENTRY_SIZE  equ 24

PML4_ADDR     equ 0x70000
PDPT_ADDR     equ 0x71000
PD_ADDR       equ 0x72000
HIGH_PT_ADDR equ 0x75000

KERNEL_ELF_BUFFER equ 0x80000

; FAT
FAT12_BPB equ 0x4000
FAT_ROOT_BUF       equ 0x4500
FAT_BUF equ 0x5000


[org 0]
[bits 16]

start:
    cli
    cld
    mov [cs:boot_drive], dl

    mov ax, cs
    mov ds, ax

    call get_e820
    call get_vbe_info
    call vesa_string
    call get_vbe_mode_info
    call set_vbe_mode
    ; call set_text_mode
    call init_boot_info
    call fat12_find_partition
    call save_boot_partition_lba
    call fat12_parse_bpb
    call fat12_read_root
    call fat12_find_kernel
    call fat12_load_kernel

    mov si, msg
    call print

    mov si, msg_fat_ok
    call print

    call enter_protected_mode

; Print

fat12_open:
    push ax
    push dx
    push si
    push es

    xor ax, ax
    mov es, ax

    mov si, DAP_FAT
    mov ah, 0x42
    int 0x13

    jc .error

    pop es
    pop si
    pop dx
    pop ax
    ret

.error:
    mov si, msg_fat_error
    call print

    cli
    hlt
    jmp $

fat12_find_partition:
    push ax
    push bx
    push cx
    push dx
    push si
    push es

    xor ax, ax
    mov es, ax

    ; MBR partition table:
    ; 0x7C00 + 0x1BE = 0x7DBE
    mov si, 0x7DBE
    mov cx, 4

.next_partition:
    ; Partition type
    mov al, [es:si + 4]

    ; Empty partition -> next
    test al, al
    jz .next

    ; Get partition start LBA
    mov eax, [es:si + 8]

    ; Save candidate
    mov [fat_partition_start], eax

    ; Put candidate LBA into DAP
    mov [DAP_FAT + 8], eax

    ; Read partition boot sector to FAT12_BPB
    call fat12_open

    ; Check filesystem type string at BPB + 0x36
    ;
    ; FAT12 boot sector contains:
    ; "FAT12   "
    ;
    cmp byte [es:FAT12_BPB + 0x36], 'F'
    jne .next
    cmp byte [es:FAT12_BPB + 0x37], 'A'
    jne .next
    cmp byte [es:FAT12_BPB + 0x38], 'T'
    jne .next
    cmp byte [es:FAT12_BPB + 0x39], '1'
    jne .next
    cmp byte [es:FAT12_BPB + 0x3A], '2'
    jne .next

    ; Found FAT12!
    pop es
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

.next:
    add si, 16
    loop .next

    ; No FAT12 partition found
    mov si, msg_fat_partition_error
    call print

    cli
    hlt
    jmp $

fat12_parse_bpb:
    push ax
    push bx
    push dx
    push es

    xor ax, ax
    mov es, ax

    ; Bytes per sector (BPB + 0x0B)
    mov ax, [es:FAT12_BPB + 0x0B]
    mov [fat_bytes_per_sector], ax

    ; Sectors per cluster (BPB + 0x0D)
    mov al, [es:FAT12_BPB + 0x0D]
    mov [fat_sectors_per_cluster], al

    ; Reserved sectors (BPB + 0x0E)
    mov ax, [es:FAT12_BPB + 0x0E]
    mov [fat_reserved_sectors], ax

    ; Number of FATs (BPB + 0x10)
    mov al, [es:FAT12_BPB + 0x10]
    mov [fat_num_fats], al

    ; Root directory entries (BPB + 0x11)
    mov ax, [es:FAT12_BPB + 0x11]
    mov [fat_root_entries], ax

    ; Sectors per FAT (BPB + 0x16)
    mov ax, [es:FAT12_BPB + 0x16]
    mov [fat_sectors_per_fat], ax

    ; root_start = partition_start + resrved + (num_fats * sectors_per_fat)
    movzx eax, word [fat_reserved_sectors]
    movzx ebx, byte [fat_num_fats]
    movzx edx, word [fat_sectors_per_fat]

    imul ebx, edx
    add eax, ebx
    add eax, [fat_partition_start]

    mov [fat_root_start], eax

    ; root_sectors = (root_entries * 32 + bytes_per_sector - 1) / bytes_per_sector
    movzx eax, word [fat_root_entries]
    shl eax, 5

    movzx ebx, word [fat_bytes_per_sector]
    add eax, ebx
    dec eax

    xor edx, edx
    div ebx

    mov [fat_root_sectors], eax

    ; data_start = root_start + root_sectors
    add eax, [fat_root_start]
    mov [fat_data_start], eax

    pop es
    pop dx
    pop bx
    pop ax

    ret

fat12_read_root:
    push ax
    push dx
    push si
    push es

    xor ax, ax
    mov es, ax

    ; 1 Root-Sector
    mov word [DAP_FAT + 2], 1

    ; Buffer = 0000:4500
    mov word [DAP_FAT + 4], FAT_ROOT_BUF
    mov word [DAP_FAT + 6], 0

    ; Root-Start LBA
    mov eax, [fat_root_start]
    mov [DAP_FAT + 8], eax

    mov si, DAP_FAT
    mov ah, 0x42
    int 0x13

    jc .error_read

    pop es
    pop si
    pop dx
    pop ax

    ret

.error_read:
    mov si, msg_fat_error
    call print

    cli
    hlt
    jmp $

fat12_find_kernel:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es

    xor ax, ax
    mov es, ax

    mov si, FAT_ROOT_BUF
    mov cx, 16

.next_entry:
    ; 0x00 = No entries
    cmp byte [es:si], 0x00
    je .not_found

    ; 0xE5 = deleted entry
    cmp byte[es:si], 0xE5
    je .next_kernel

    ; Only normal files
    mov al, [es:si + 11]

    ; Dir?
    test al, 0x10
    jnz .next_kernel

    ; Compare 11 chars
    mov di, kernel_filename
    push si
    push cx

    mov bx, 11

.compare:
    mov al, [es:si]
    cmp al, [di]
    jne .no_match

    inc si
    inc di
    dec bx
    jnz .compare

    ; Found
    pop cx
    pop si

    ; Startcluster
    mov ax, [es:si + 26]
    mov [fat_kernel_cluster], ax

    ; Filesize
    mov eax, [es:si + 28]
    mov [fat_kernel_size], eax

    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax

    ret

.no_match:
    pop cx
    pop si

.next_kernel:
    add si, 32
    loop .next_entry

.not_found:
    mov si, msg_kernel_not_found
    call print

    cli
    hlt
    jmp $

fat12_load_kernel:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es

    xor ax, ax
    mov es, ax

    mov ax, [fat_sectors_per_fat]
    mov [DAP_FAT + 2], ax

    mov word [DAP_FAT + 4], FAT_BUF
    mov word [DAP_FAT + 6], 0

    mov eax, [fat_partition_start]
    add eax, 1
    mov [DAP_FAT + 8], eax

    mov si, DAP_FAT
    mov ah, 0x42
    int 0x13

    jc .fat_error

    mov word [fat_load_segment], 0x8000
    mov dword [fat_loaded_bytes], 0

    mov bx, [fat_kernel_cluster]

.next_cluster:
    mov eax, [fat_loaded_bytes]
    cmp eax, [fat_kernel_size]
    jae .done_load

    movzx eax, bx
    sub eax, 2

    movzx ecx, byte [fat_sectors_per_cluster]
    imul eax, ecx

    add eax, [fat_data_start]

    mov [DAP_FAT + 8], eax

    movzx eax, byte [fat_sectors_per_cluster]
    movzx ecx, word [fat_bytes_per_sector]
    imul eax, ecx

    mov edx, [fat_kernel_size]
    sub edx, [fat_loaded_bytes]

    cmp edx, eax
    jae .full_cluster

    mov eax, edx

.full_cluster:
    movzx ecx, word [fat_bytes_per_sector]
    xor edx, edx
    div ecx

    test edx, edx
    jz .sector_count_ready
    inc eax

.sector_count_ready:
    mov [DAP_FAT + 2], ax

    mov word [DAP_FAT + 4], 0

    mov ax, [fat_load_segment]
    mov [DAP_FAT + 6], ax

    mov dl, [boot_drive]
    mov si, DAP_FAT
    mov ah, 0x42
    int 0x13

    jc .kernel_error

    xor eax, eax
    mov ax, [DAP_FAT + 2]

    movzx ecx, word [fat_bytes_per_sector]
    imul eax, ecx

    add [fat_loaded_bytes], eax

    movzx eax, word [DAP_FAT + 2]
    movzx ecx, word [fat_bytes_per_sector]
    imul eax, ecx

    shr eax, 4

    add [fat_load_segment], ax

    movzx eax, bx
    mov edx, eax
    shr edx, 1
    add eax, edx

    mov si, FAT_BUF
    add si, ax

    mov ax, [es:si]

    test bx, 1
    jnz .odd_cluster

    and ax, 0x0FFF
    jmp .next_load

.odd_cluster:
    shr ax, 4

.next_load:

    mov bx, ax

    ; FAT12 End Of Chain
    cmp bx, 0xFF8
    jae .done_load

    ; Invalid cluster
    cmp bx, 2
    jb .kernel_error

    jmp .next_cluster


.done_load:
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret


.fat_error:
    mov [fat_disk_error], ah

    mov si, msg_fat_table_error
    call print

    cli
    hlt
    jmp $


.kernel_error:
    mov [fat_disk_error], ah

    mov si, msg_kernel_read_error
    call print

    mov al, [fat_disk_error]
    call print_hex8

    cli
    hlt
    jmp $

print_hex8:
    push ax
    push ax
    shr al, 4
    call print_hex_digit
    pop ax
    and al, 0x0F
    call print_hex_digit
    pop ax
    ret

print_hex_digit:
    cmp al, 10
    jb .number

    add al, 'A' - 10
    jmp .print_hex

.number:
    add al, '0'

.print_hex:
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    ret

print:
    push ax
    push si

.next_print:
    lodsb

    test al, al
    jz .done

    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0F
    int 0x10

    jmp .next_print

.done:
    pop si
    pop ax
    ret

vesa_string:
    xor ax, ax
    mov ds, ax

    mov si, [BOOT_VBE_CTRL + 0x06]
    mov ax, [BOOT_VBE_CTRL + 0x08]
    mov ds, ax

    xor ax, ax
    mov es, ax
    mov di, BOOT_VBE_OEM

.copy_oem:
    lodsb
    stosb

    test al, al
    jnz .copy_oem

    mov ax, cs
    mov ds, ax

    ret

get_vbe_info:
    mov ax, 0
    mov es, ax

    mov ax, 0x4F00
    mov di, BOOT_VBE_CTRL
    int 0x10

    cmp ax, 0x004F
    jne vbe_fail

    ret

get_vbe_mode_info:
    mov ax, 0
    mov es, ax

    mov ax, 0x4F01
    mov cx, 0x118
    mov di, BOOT_VBE_MODE

    int 0x10

    cmp ax, 0x004F
    jne vbe_fail

    ret

vbe_fail:
    mov byte [video_mode], 0
    ret

set_text_mode:
    mov ax, 0x0003
    int 0x10

    mov byte [video_mode], 0

    ret

set_vbe_mode:
    mov ax, 0x4F02
    mov bx, 0x4118
    int 0x10

    cmp ax, 0x004F
    jne vbe_fail

    mov byte [video_mode], 1
    ret

get_e820:
    pusha
    push ds
    push es

    mov ax, cs
    mov ds, ax

    xor ax, ax
    mov [e820_entries], ax

    xor ax, ax
    mov es, ax
    mov di, BOOT_E820

    xor ebx, ebx

.e820_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, E820_ENTRY_SIZE

    int 0x15

    jc .e820_done

    cmp eax, 0x534D4150
    jne .e820_done

    inc word [e820_entries]

    add di, E820_ENTRY_SIZE

    cmp word [e820_entries], MAX_E820_ENTRIES
    jae .e820_done

    test ebx, ebx
    jnz .e820_loop

.e820_done:
    mov ax, [e820_entries]
    mov [es:BOOT_INFO_ADDR + 0x1C], ax

    pop es
    pop ds
    popa
    ret

; Boot Info
init_boot_info:
    xor ax, ax
    mov es, ax

    mov dword [es:BOOT_INFO_ADDR + 0x00], 0x4B545959
    mov dword [es:BOOT_INFO_ADDR + 0x04], 1

    movzx eax, byte [video_mode]
    mov [es:BOOT_INFO_ADDR + 0x08], eax

    movzx eax, word [es:BOOT_VBE_MODE + 0x12]
    mov [es:BOOT_INFO_ADDR + 0x0C], eax

    movzx eax, word [es:BOOT_VBE_MODE + 0x14]
    mov [es:BOOT_INFO_ADDR + 0x10], eax

    movzx eax, word [es:BOOT_VBE_MODE + 0x19]
    mov [es:BOOT_INFO_ADDR + 0x14], eax

    mov eax, [es:BOOT_VBE_MODE + 0x28]
    mov [es:BOOT_INFO_ADDR + 0x18], eax

    mov al, [boot_drive]
    mov [es:BOOT_INFO_ADDR + 0xA20], al

    ; MBR Disk Signature (0x7C00 + 0x1B8)
    mov eax, [es:0x7DB8]
    mov [es:BOOT_INFO_ADDR + 0xA29], eax

    ret

save_boot_partition_lba:
    xor ax, ax
    mov es, ax

    mov eax, [cs:fat_partition_start]

    mov [es:BOOT_INFO_ADDR + 0xA21], eax
    mov dword [es:BOOT_INFO_ADDR + 0xA25], 0

    ret

; copy e820

fail:
    mov ah, 0x0E
    mov al, 'E'
    int 0x10

; Protected Mode

gdt_start:

    ; Null descriptor
    dq 0

    ; Code descriptor - 0x08
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

    ; Data descriptor - 0x10
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

    ; 64-bit Code descriptor
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 10101111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd STAGE2_BASE + gdt_start

enter_protected_mode:
    cli

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:(STAGE2_BASE + protected_mode_entry)


[bits 32]

protected_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Stack
    mov esp, 0x90000

    call setup_paging

    ; Enter 64-bit code segment
    jmp 0x18:(STAGE2_BASE + long_mode_entry)


setup_paging:
    ; Clear PML4
    mov edi, PML4_ADDR
    xor eax, eax
    mov ecx, 512

.clear_pml4:
    mov [edi], eax
    mov [edi + 4], eax
    add edi, 8
    loop .clear_pml4

    ; Clear PDPT
    mov edi, PDPT_ADDR
    xor eax, eax
    mov ecx, 512

.clear_pdpt:
    mov [edi], eax
    mov [edi + 4], eax
    add edi, 8
    loop .clear_pdpt

    ; Clear normal PD
    mov edi, PD_ADDR
    xor eax, eax
    mov ecx, 512

.clear_pd:
    mov [edi], eax
    mov [edi + 4], eax
    add edi, 8
    loop .clear_pd

    ; Clear HIGH_PDPT
    mov edi, HIGH_PDPT_ADDR
    xor eax, eax
    mov ecx, 512

.clear_high_pdpt:
    mov [edi], eax
    mov [edi + 4], eax
    add edi, 8
    loop .clear_high_pdpt

    ; Clear HIGH_PD
    mov edi, HIGH_PD_ADDR
    xor eax, eax
    mov ecx, 512

.clear_high_pd:
    mov [edi], eax
    mov [edi + 4], eax
    add edi, 8
    loop .clear_high_pd

    ; Clear HIGH_PT
    mov edi, HIGH_PT_ADDR
    xor eax, eax
    mov ecx, 512

.clear_high_pt:
    mov [edi], eax
    mov [edi + 4], eax
    add edi, 8
    loop .clear_high_pt

    ; Identity mapping: first 1 GiB
    mov edi, PML4_ADDR
    mov eax, PDPT_ADDR
    or eax, 0x003
    mov [edi], eax
    mov dword [edi + 4], 0

    mov edi, PDPT_ADDR
    mov eax, PD_ADDR
    or eax, 0x003
    mov [edi], eax
    mov dword [edi + 4], 0

    ; 512 x 2 MiB identity pages
    mov edi, PD_ADDR
    xor eax, eax
    mov ecx, 512

.map_identity_2m:
    mov edx, eax
    or edx, 0x083

    mov [edi], edx
    mov dword [edi + 4], 0

    add eax, 0x200000
    add edi, 8
    loop .map_identity_2m

    ; Higher Half:
    ; Virtual  0xFFFFFFFF80000000
    ; Physical 0x00100000

    ; PML4[511] -> HIGH_PDPT
    mov edi, PML4_ADDR + (511 * 8)
    mov eax, HIGH_PDPT_ADDR
    or eax, 0x003
    mov [edi], eax
    mov dword [edi + 4], 0

    ; HIGH_PDPT[510] -> HIGH_PD
    mov edi, HIGH_PDPT_ADDR + (510 * 8)
    mov eax, HIGH_PD_ADDR
    or eax, 0x003
    mov [edi], eax
    mov dword [edi + 4], 0

    ; HIGH_PD[0] -> HIGH_PT
    mov edi, HIGH_PD_ADDR
    mov eax, HIGH_PT_ADDR
    or eax, 0x003
    mov [edi], eax
    mov dword [edi + 4], 0

    ; Map 512 x 4 KiB pages:
    ; 0xFFFFFFFF80000000 -> 0x00100000
    mov edi, HIGH_PT_ADDR
    mov eax, 0x00100000
    mov ecx, 512

.map_high_4k:
    mov edx, eax
    or edx, 0x003

    mov [edi], edx
    mov dword [edi + 4], 0

    add eax, 0x1000
    add edi, 8
    loop .map_high_4k

    ; Load PML4
    mov eax, PML4_ADDR
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    ; Enable Long Mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr

    ; Enable Paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ret

[bits 64]

long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov rsp, 0x9F000

    mov rbx, KERNEL_ELF_BUFFER
    mov rax, [rbx + 0x20]
    add rbx, rax

    cmp dword [rbx], 1
    jne .fail

    mov rsi, KERNEL_ELF_BUFFER

    add rsi, [rbx + 0x08]
    mov rdi, [rbx + 0x18]
    mov rcx, [rbx + 0x20]
    cld
    rep movsb

    mov rcx, [rbx + 0x28]
    sub rcx, [rbx + 0x20]
    xor al, al
    rep stosb

    mov rax, [KERNEL_ELF_BUFFER + 0x18]
    ; hlt
    jmp rax

.fail:
    mov rax, 1
    hlt

e820_entries: dw 0

video_mode: db 0
msg: db "Booting...", 13, 10, 0
msg_fat_error: db "FAT12 read error!", 13, 10, 0
msg_fat_table_error: db "FAT Table read error!", 13, 10, 0
msg_kernel_read_error: db "Kernel read error!", 13, 10, 0
msg_fat_ok: db "FAT12 opended!", 13, 10, 0
msg_fat_partition_error: db "No FAT12 partition!", 13, 10, 0
msg_kernel_not_found: db "kernel.elf not found!", 13, 10, 0

; FAT
fat_bytes_per_sector:    dw 0
fat_sectors_per_cluster: db 0
fat_reserved_sectors:    dw 0
fat_num_fats:             db 0
fat_root_entries:         dw 0
fat_sectors_per_fat:     dw 0

fat_root_start:           dd 0
fat_root_sectors:         dd 0
fat_data_start:           dd 0
fat_partition_start: dd 0
fat_kernel_cluster:   dw 0
fat_kernel_size:      dd 0
fat_load_segment: dw 0x8000
fat_loaded_bytes: dd 0

boot_drive: db 0

fat_disk_error: db 0

DAP_FAT:
    db 0x10
    db 0x00
    dw 0x0001          ; 1 sector
    dw FAT12_BPB       ; offset
    dw 0x0000          ; segment
    dq 0               ; partition start LBA

; Kernel
kernel_filename: db "KERNEL  ELF"