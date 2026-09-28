#include <stdint.h>
#include <vga.h>
#include <string.h>
#include <keyboard.h>
#include <ports.h>
#include <bios.h>
#include <fat.h>
#include <cd.h>
#include <kernel.h>
#include <colors.h>
#include <cpu.h>
#include <logs.h>
#include <vbe.h>
#include <hex_colors.h>
#include <tools.h>
#include <vfs.h>
#include <kprint.h>
#include <heap.h>

volatile int shell_abort = 0;
int ctrl_down = 0;

int shell_cursor_x = 0;
int shell_cursor_y = 0;
int prompt_len = 0;

extern int kcursor_x;
extern int kcursor_y;

/*

const char logo[] =
    " /\\_/\\ \n"
    "( o.o )\n"
    " > ^ < \n"
    "Copyright (C) KittyyOS\n";

const char KittyyOS[] =
    GREEN "OS: " WHITE "KittyyOS Version 1.0\n"
    BLUE "Kernel: " WHITE "KittyyOS Kernel 1.0\n"
    RED "Bootloader: " WHITE "KittyyOS BIOS Bootloader 1.0\n";

const char help[] =
    "Avaiable commands:\n"
    "  clear               - Clear the screen\n"
    "  shutdown            - Power off the system\n"
    "  os                  - Show OS information\n"
    "  ls                  - List directory contents\n"
    "  cd <dir>            - Change directory\n"
    "  idt                 - Show IDT info\n"
    "  gdt                 - Show GDT info\n"
    "  fg <color>          - Change Foreground\n"
    "  bg <color>          - Change Background\n"
    "  x/<n>i <addr>       - Dump <n> bytes at <addr>\n"
    "  show <file>         - Show file\n";

void ram() {
    double gb = (double)mb / 1024.0;
    kprintf(RED "Memory: " WHITE "%fGB\n", gb);

    int heapused = heap_used();
    int heapsize = heap_size();

    kprintf(GREEN "Heap Size: " WHITE "%d/%d\n", heapused, heapsize);
}

void dump_raw(void *addr, int count) {
    unsigned char *p = addr;

    for (int i = 0; i < count; i++) {
        kprintf("0x%llx: %02x\n", (unsigned long long)(p+i), p[i]);
    }
}

int instr_len(unsigned char *p) {
    int i = 0;

    while (p[i] == 0x66 || p[i] == 0x67 || p[i] == 0xF3 || p[i] == 0xF2) {
        i++;
    }

    unsigned char opcode = p[i++];

    if (opcode == 0x90) return i; // NOP
    if (opcode == 0xCC) return i; // INT3
    if ((opcode & 0xF0) == 0x70) return i+1; // Jcc short
    if (opcode == 0xE8) return i+4; // CALL rel32
    if (opcode == 0xE9) return i+4; // JMP rel32

    return i; // fallback
}

void dump_fake_disasm(void *addr, int count) {
    unsigned char *p = addr;
    for (int i = 0; i < count; i++) {
        int len = instr_len(p);
        kprintf("0x%llx: ", (unsigned long long)p);
        for (int j = 0; j < len; j++)
        kprintf("%02x ", p[j]);
        kprintf("\n");
        p += len;
    }
}

int atoi(const char *s) {
    int n = 0;
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    return n;
}

uint64_t strtoull(const char *s, char **end, int base) {
    uint64_t n = 0;

    // Hex only
    if (base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s +=2;
        }

        while ((*s >= '0' && *s <= '9') ||
               (*s >= 'a' && *s <= 'f') ||
               (*s >= 'A' && *s <= 'F')) {

            n *= 16;

            if (*s >= '0' && *s <= '9')
                n += *s - '0';
            else if (*s >= 'a' && *s <= 'f')
                n += *s - 'a' + 1 + 9;
            else
                n += *s - 'A' + 1 + 9;

            s++;
        }
    }

    if (end) *end = (char*)s;
    return n;
}

void reboot() {
    asm volatile("cli");

    while (inb(0x64) & 0x02);

    outb(0x64, 0xFE);

    for (;;)
        asm volatile("hlt");
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

    kprintf("root@%s:[%s]-> ", g_hostname, cwd);

    shell_cursor_x = get_cursor_x();   
    shell_cursor_y = get_cursor_y();
    prompt_len = get_cursor_x();

    shell_readline(input, 64);

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
        kclear_screen();
        return 0;
    }

    if (strcmp(command, "shutdown") == 0) {
        kprintf("Shuting down...\n");
        acpi_shutdown();
        return 0;
    }

    if (strcmp(command, "os") == 0) {
        char brand[49];
        char vendor[13];

        cpu_brand(brand);
        cpu_vendor(vendor);

        kprintf("%s", logo);
        kprintf("%s", KittyyOS);
        kprintf(GREEN "CPU Vendor: " WHITE "%s\n", vendor);
        kprintf(BLUE "CPU Brand: " WHITE "%s\n", brand);
        ram();
        return 0;
    }

    if (strcmp(command, "ls") == 0) {
        fat32_ls();
        return 0;
    }

    if (strcmp(command, "show") == 0) {
        fat32_show(args);
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

    if (strcmp(command, "help") == 0) {
        kprintf("%s", help);
        return 0;
    }

    if (strncmp(command, "x/", 2) == 0) {
        int count = atoi(command + 2);
        char mode = command[strlen(command) - 1];
        uint64_t addr = strtoull(args, NULL, 16);

        dump_raw((void*)addr, count);

        return 0;
    }

    kprintf("Unknown command: %s\n", command);

    return 0;
} */