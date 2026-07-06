#include <stdint.h>
#include <vga.h>
#include <string.h>
#include <keyboard.h>
#include <ports.h>
#include <bios.h>
#include <fat.h>
#include <cd.h>
#include <kernel.h>

int shell_cursor_x = 0;
int shell_cursor_y = 0;
int prompt_len = 0;

void reboot() {
    asm volatile("lidt 0");
    asm volatile("int $3");
}

int shell() {
    char input[64];
    char command[32];
    char args[64];
    char cwd[64];

    fat32_get_current_dir_name(cwd);

    if (strcmp(cwd, "home") == 0) {
        strcpy(cwd, "~");
    }

    if (strcmp(cwd, "/") == 0) {
        strcpy(cwd, "/");
    }

    vga_printf("root@KittyyOS:[%s]-> ", cwd);

    shell_cursor_x = cursor_x;   
    shell_cursor_y = cursor_y;
    prompt_len = shell_cursor_x;

    shell_readline(input, 64);

    // ---------------------------
    // COMMAND + ARGUMENT PARSER
    // ---------------------------
    int i = 0, j = 0;

    // Befehl extrahieren
    while (input[i] != ' ' && input[i] != 0) {
        command[j++] = input[i++];
    }
    command[j] = 0;

    // Argumente extrahieren
    if (input[i] == ' ') i++;

    j = 0;
    while (input[i] != 0) {
        args[j++] = input[i++];
    }
    args[j] = 0;

    if (strcmp(command, "clear") == 0) {
        clear_screen();
        return 0;
    }

    if (strcmp(command, "shutdown") == 0) {
        vga_printf("Shuting down...\n");
        acpi_shutdown();
        return 0;
    }

    if (strcmp(command, "reboot") == 0) {
        reboot();
        return 0;
    }

    if (strcmp(command, "ls") == 0) {
        fat32_ls();
        return 0;
    }

    if (strcmp(command, "cd") == 0) {
        fat32_cd(args);
        return 0;
    }

    if (strcmp(command, "idt") == 0) {
        check_idt();
        return 0;
    }

    if (strcmp(command, "gdt") == 0) {
        check_gdt();
        return 0;
    }

    // ---------------------------
    // UNKNOWN COMMAND
    // ---------------------------
    vga_printf("Unknown command: %s\n", command);

    return 0;
}
