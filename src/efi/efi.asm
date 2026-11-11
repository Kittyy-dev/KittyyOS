section .text

global efi_entry
extern efi_prepare_stack
extern efi_main

efi_entry:
    ; Alten Stack und Register sichern
    sub rsp, 72

    mov [rsp + 32], rcx
    mov [rsp + 40], rdx

    mov [rsp + 48], r12
    mov [rsp + 56], r13
    mov [rsp + 64], r14

    ; Alten Stack merken
    lea r12, [rsp + 72]

    ; Stack vorbereiten
    call efi_prepare_stack

    ; RAX = neuer Stack
    test rax, rax
    jz .stack_failed

    ; Neuer Stack
    mov rsp, rax

    ; 16-Byte Alignment + Shadow Space
    and rsp, -16
    sub rsp, 32

    ; ImageHandle und SystemTable wiederherstellen
    mov rcx, [r12 - 40]
    mov rdx, [r12 - 32]

    call efi_main

    ; Alten Stack wiederherstellen
    mov rsp, r12

    mov r12, [rsp - 24]
    mov r13, [rsp - 16]
    mov r14, [rsp - 8]

    add rsp, 72
    ret

.stack_failed:
    mov rsp, r12

    mov r12, [rsp - 24]
    mov r13, [rsp - 16]
    mov r14, [rsp - 8]

    add rsp, 72

    ; EFI_OUT_OF_RESOURCES
    mov rax, 0x8000000000000009
    ret