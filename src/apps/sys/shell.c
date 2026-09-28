#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel_api.h>
#include <colors.h>
#include <task.h>

extern struct task *task_create_name(task_entry_t entry, const char *name);
extern void yield(void);
// extern void atom_test(KernelAPI *api);

volatile int shell_abort = 0;
int ctrl_down = 0;

int shell_cursor_x = 0;
int shell_cursor_y = 0;
int prompt_len = 0;

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
    "    clear          -    Clear the screen\n"
    "    shutdown       -    Power off the system\n"
    "    os             -    Show OS information\n"
    "    ls             -    List directory contents\n"
    "    cd <dir>       -    Change directory\n"
    "    show <file>    -    Show file\n";

void ram(KernelAPI *api) {
    double gb = (double)api->get_memory_mb() / 1024.0;

    api->kprintf(RED "Memory: " WHITE "%fGB\n", gb);

    uint32_t heapused = api->heap_used();
    uint32_t heapsize = api->heap_size();

    api->kprintf(GREEN "Heap Size: " WHITE "%d/%d\n", heapused, heapsize);
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

    if (base == 16) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s += 2;
        }

        while ((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F')) {
            n *= 16;

            if (*s >= '0' && *s <= '9') {
                n += *s - '0';
            } else if (*s >= 'a' && *s <= 'f') {
                n += *s - 'a' + 10;
            } else {
                n += *s - 'A' + 10;
            }

            s++;
        }
    }

    if (end) {
        *end = (char *)s;
    }

    return n;
}

void reboot(KernelAPI *api) {
    asm volatile("cli");

    while (api->inb(0x64) & 0x02);

    api->outb(0x64, 0xFE);

    for (;;) {
        asm volatile("hlt");
    }
}

void shell(KernelAPI *api) {
    char input[64];
    char command[32];
    char args[64];
    
    vfs_node_t *cwd = api->vfs_get_cwd();

    if (cwd) {
        if (cwd == api->vfs_get_root()) {
            api->kprintf("root@%s:[/]-> ", api->hostname);
        } else {
            api->kprintf("root@%s:[%s]-> ", api->hostname, cwd->name);
        }
    }

    shell_cursor_x = api->get_cursor_x();
    shell_cursor_y = api->get_cursor_y();
    prompt_len = api->get_cursor_x();

    api->shell_readline(input, 64);

    int i = 0, j = 0;

    while (input[i] != ' ' && input[i] != 0) {
        command[j++] = input[i++];
    }

    command[j] = 0;

    if (input[i] == ' ') {
        i++;
    }

    j = 0;

    while (input[i] != 0) {
        args[j++] = input[i++];
    }

    args[j] = 0;

    if (api->strcmp(command, "clear") == 0) {
        api->kclear_screen();
        return;
    }

    if (api->strcmp(command, "shutdown") == 0) {
        api->kprintf("Shuting down...\n");
        api->acpi_shutdown();
        return;
    }

    if (api->strcmp(command, "os") == 0) {
        char brand[49];
        char vendor[13];

        api->cpu_brand(brand);
        api->cpu_vendor(vendor);

        api->kprintf("%s", logo);
        api->kprintf("%s", KittyyOS);
        api->kprintf(BLUE "CPU Brand: " WHITE "%s\n", brand);
        return;
    }

    if (api->strcmp(command, "ls") == 0) {
        vfs_node_t* dir;

        if (args[0] == '\0') {
            dir = api->vfs_get_cwd();
        } else {
            dir = api->vfs_resolve_path(args);
        }

        if (!dir) {
            api->kprintf("ls: No such directory!\n");
            return;
        }

        api->vfs_ls(dir);
        return;
    }

    if (api->strcmp(command, "show") == 0) {
        vfs_node_t* node = api->vfs_resolve_path(args);
        char buf[8192];
        size_t n = api->vfs_read(node, buf, sizeof(buf) - 1);
        buf[n] = '\0';
        api->kprintf("%s", buf);
        return;
    }

    if (api->strcmp(command, "cd") == 0) {
        if (!api->vfs_cd(args)) {
            api->kprintf("cd %s: No such directory\n", args);
        }
        return;
    }

    if (api->strcmp(command, "help") == 0) {
        api->kprintf("%s", help);
        return;
    }

    if (api->strcmp(command, "write") == 0) {
        char *space = args;

        while (*space && *space != ' ') {
            space++;
        }

        if (*space == '\0') {
            api->kprintf("Usage: write <file> <text>\n");
            return;
        }

        *space = '\0';
        char *filename = args;
        char *text = space + 1;

        while (*text == ' ') {
            text++;
        }

        vfs_node_t *file = api->vfs_resolve_path(filename);

        if (!file) {
            api->kprintf("wirte: file not found: %s\n", filename);
            return;
        }

        if (file->is_dir) {
            api->kprintf("write: %s is a directory\n", filename);
            return;
        }

        size_t len = 0;
        while (text[len]) {
            len++;
        }

        size_t written = api->vfs_write(file, text, len);

        if (written != len) {
            api->kprintf("write: only wrote %d/%d bytes\n", (int)written, (int)len);

            return;
        }

        api->kprintf("Wrote %d bytes to %s\n", (int)written, filename);

        return;
    }

    if (api->strcmp(command, "run") == 0) {
        api->kernel_load_module(args);
        return;
    }

    api->kprintf("Unknown command: %s\n", command);

    return;
}