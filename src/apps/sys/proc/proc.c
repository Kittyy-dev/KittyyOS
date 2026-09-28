#include "vfs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel_api.h>
#include <proc.h>
#include <colors.h>
#include <scheduler.h>
#include <task.h>

#define TSC_HZ 1000000000ULL

// KernelAPI
static KernelAPI* proc_api = NULL;
static uint64_t boot_tsc = 0;
static vfs_node_t* proc_root = NULL;

// Proc node types

typedef enum {
    PROC_ROOT,
    PROC_UPTIME,
    PROC_MEMINFO,
    PROC_CPUINFO,
    PROC_TASK_DIR,
    PROC_TASK_STATUS,
    PROC_TASK_CMDLINE
} proc_type_t;

// Proc internal data

typedef struct {
    proc_type_t type;
    struct task* task;
} proc_node_t;

static vfs_node_t* proc_add_node(vfs_node_t* parent, const char* name, bool is_dir, proc_type_t type, struct task* task) {
    vfs_node_t* node = proc_api->kmalloc(sizeof(vfs_node_t));

    if (!node) {
        return NULL;
    }

    proc_api->memset(node, 0, sizeof(vfs_node_t));

    node->name = proc_api->strdup(name);

    if (!node->name) {
        proc_api->free(node);
        return NULL;
    }

    node->is_dir = is_dir;
    node->parent = parent;
    node->fs = parent->fs;

    proc_node_t* internal = proc_api->kmalloc(sizeof(proc_node_t));

    if (!internal) {
        proc_api->free(node->name);
        proc_api->free(node);
        return NULL;
    }

    internal->type = type;
    internal->task = task;

    node->internal = internal;

    // Add to vfs child list
    node->next = parent->children;
    parent->children = node;

    return node;
}

static void* proc_mount(void)
{
    proc_node_t* internal =
        proc_api->kmalloc(sizeof(proc_node_t));

    if (!internal) {
        return NULL;
    }

    internal->type = PROC_ROOT;
    internal->task = NULL;

    return internal;
}

static vfs_node_t* proc_create_file(vfs_node_t* dir, const char* name) {
    (void)dir;
    (void)name;

    return NULL;
}

static vfs_node_t* proc_create_dir(vfs_node_t* dir, const char* name) {
    (void)dir;
    (void)name;

    return NULL;
}

static size_t proc_copy_string(void* buf, size_t len, const char* str) {
    size_t size = proc_api->strlen(str);

    if (size > len) {
        size = len;
    }

    proc_api->memcpy(buf, str, size);

    return size;
}

static size_t proc_format_uptime(char* buf, size_t len, uint64_t seconds, uint64_t centiseconds) {
    if (!buf || len == 0) {
        return 0;
    }

    char tmp[32];
    size_t pos = 0;

    // Seconds to Decimal
    char digits[20];
    size_t digit_count = 0;

    if (seconds == 0) {
        digits[digit_count++] = '0';
    } else {
        while (seconds > 0 && digit_count < sizeof(digits)) {
            digits[digit_count++] = '0' + (seconds % 10);

            seconds /= 10;
        }
    }

    // Write to buffer

    while (digit_count > 0 && pos + 1 < len) {
        buf[pos++] = digits[--digit_count];
    }
    
    if (pos + 1 < len) {
        buf[pos++] = '.';
    }

    if (pos + 1 < len) {
        buf[pos++] = '0' + (centiseconds / 10);
    }

    if (pos + 1 < len) {
        buf[pos++] = '0' + (centiseconds % 10);
    }

    if (pos + 1 < len) {
        buf[pos++] = ' ';
    }

    if (pos + 1 < len) {
        buf[pos++] = '0';
    }

    if (pos + 1 < len) {
        buf[pos++] = '.';
    }

    if (pos + 1 < len) {
        buf[pos++] = '0';
    }

    if (pos + 1 < len) {
        buf[pos++] = '0';
    }

    if (pos + 1 < len) {
        buf[pos++] = '\n';
    }

    buf[pos] = '\0';

    return pos;
}

static void proc_putc(char* buf, size_t len, size_t *pos, char c) {
    if (*pos + 1 >= len) {
        return;
    }

    buf[(*pos)++] = c;
}

static void proc_puts(char* buf, size_t len, size_t *pos, const char *str) {
    if (!str) {
        return;
    }

    while (*str && *pos + 1 < len) {
        buf[(*pos)++] = *str++;
    }
}

static void proc_uint_to_string(char *buf, size_t len, uint64_t value) {
    if (!buf || len == 0) {
        return;
    }

    size_t pos = 0;
    char tmp[32];
    size_t n = 0;

    if (value == 0) {
        buf[0] = '0';
        if (len > 1) {
            buf[1] = '\0';
        }
        return;
    }

    while (value > 0 && n < sizeof(tmp)) {
        tmp[n++] = '0' + (value % 10);
        value /= 10;
    }

    while (n > 0 && pos + 1 < len) {
        buf[pos++] = tmp[--n];
    }

    buf[pos] = '\0';
}

static void proc_put_uint(char *buf, size_t len, size_t *pos, uint64_t value) {
    char tmp[32];
    size_t n = 0;

    if (value == 0) {
        proc_putc(buf, len, pos, '0');
        return;
    }

    while (value > 0 && n < sizeof(tmp)) {
        tmp[n++] = '0' + (value % 10);
        value /= 10;
    }

    while (n > 0) {
        proc_putc(buf, len, pos, tmp[--n]);
    }
}

static void proc_put_octal(char *buf, size_t len, size_t *pos, uint16_t value) {
    char tmp[16];
    size_t n = 0;

    if (value == 0) {
        proc_putc(buf, len, pos, '0');
        proc_putc(buf, len, pos, '0');
        proc_putc(buf, len, pos, '0');
        proc_putc(buf, len, pos, '0');
        return;
    }

    while (value > 0 && n < sizeof(tmp)) {
        tmp[n++] = '0' + (value & 7);
        value >>= 3;
    }

    while (n < 4) {
        tmp[n++] = '0';
    }

    while (n > 0) {
        proc_putc(buf, len, pos, tmp[--n]);
    }
}

static void proc_populate_tasks(vfs_node_t* root)
{
    struct task* tasks = scheduler_get_tasks();
    size_t count = scheduler_get_task_count();

    for (size_t i = 0; i < count; i++) {
        struct task* task = &tasks[i];

        if (task->state == TASK_UNUSED)
            continue;

        char pid[32];

        // PID in String umwandeln
        size_t pos = 0;
        proc_put_uint(pid, sizeof(pid), &pos, task->id);
        proc_putc(pid, sizeof(pid), &pos, '\0');

        vfs_node_t* dir = proc_add_node(
            root,
            pid,
            true,
            PROC_TASK_DIR,
            task
        );

        if (!dir)
            continue;

        proc_add_node(
            dir,
            "status",
            false,
            PROC_TASK_STATUS,
            task
        );

        proc_add_node(
            dir,
            "cmdline",
            false,
            PROC_TASK_CMDLINE,
            task
        );
    }
}

static size_t proc_meminfo(char *buf, size_t len) {
    if (!buf || len == 0 || !proc_api) {
        return 0;
    }

    size_t pos = 0;

    uint64_t total_mb = proc_api->get_memory_mb();
    uint64_t total_kb = total_mb * 1024;

    uint64_t heap_used = proc_api->heap_used();
    uint64_t heap_size = proc_api->heap_size();

    uint64_t used_kb = heap_used / 1024;
    uint64_t heap_kb = heap_size / 1024;

    uint64_t free_kb = 0;

    if (heap_kb > used_kb) {
        free_kb = heap_kb - used_kb;
    }

    proc_puts(buf, len, &pos, "MemTotal:       ");
    proc_put_uint(buf, len, &pos, total_kb);
    proc_puts(buf, len, &pos, " kB\n");

    proc_puts(buf, len, &pos, "MemFree:        ");
    proc_put_uint(buf, len, &pos, free_kb);
    proc_puts(buf, len, &pos, " kB\n");

    proc_puts(buf, len, &pos, "MemAvailable:   ");
    proc_put_uint(buf, len, &pos, free_kb);
    proc_puts(buf, len, &pos, " kB\n");

    proc_puts(buf, len, &pos, "Buffers:        0 kB\n");
    proc_puts(buf, len, &pos, "Cached:         0 kB\n");
    proc_puts(buf, len, &pos, "SwapCached:     0 kB\n");
    proc_puts(buf, len, &pos, "Active:         ");
    proc_put_uint(buf, len, &pos, used_kb);
    proc_puts(buf, len, &pos, " kB\n");

    proc_puts(buf, len, &pos, "Inactive:       0 kB\n");
    proc_puts(buf, len, &pos, "SwapTotal:      0 kB\n");
    proc_puts(buf, len, &pos, "SwapFree:       0 kB\n");

    return pos;
}

static size_t proc_cpuinfo(char *buf, size_t len) {
    if (!buf || len == 0 || !proc_api)
        return 0;

    size_t pos = 0;

    uint32_t eax, ebx, ecx, edx;

    /* CPUID leaf 1 */
    proc_api->cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    uint32_t stepping = eax & 0xF;
    uint32_t model = (eax >> 4) & 0xF;
    uint32_t family = (eax >> 8) & 0xF;

    uint32_t ext_model = (eax >> 16) & 0xF;
    uint32_t ext_family = (eax >> 20) & 0xFF;

    if (family == 0xF)
        family += ext_family;

    if (family == 0x6 || family == 0xF)
        model += ext_model << 4;

    uint32_t apicid = (ebx >> 24) & 0xFF;

    /* CPU Name */
    char vendor[13];
    char brand[49];

    proc_api->memset(vendor, 0, sizeof(vendor));
    proc_api->memset(brand, 0, sizeof(brand));

    proc_api->cpu_vendor(vendor);
    proc_api->cpu_brand(brand);

    /* Header */

    proc_puts(buf, len, &pos, "processor    : 0\n");

    proc_puts(buf, len, &pos, "vendor_id    : ");
    proc_puts(buf, len, &pos, vendor);
    proc_putc(buf, len, &pos, '\n');


    proc_puts(buf, len, &pos, "cpu family   : ");
    proc_put_uint(buf, len, &pos, family);
    proc_putc(buf, len, &pos, '\n');

    proc_puts(buf, len, &pos, "model        : ");
    proc_put_uint(buf, len, &pos, model);
    proc_puts(buf, len, &pos, "\n");

    proc_puts(buf, len, &pos, "model name   : ");
    proc_puts(buf, len, &pos, brand);
    proc_putc(buf, len, &pos, '\n');

    proc_puts(buf, len, &pos, "stepping     : ");
    proc_put_uint(buf, len, &pos, stepping);
    proc_putc(buf, len, &pos, '\n');

    proc_puts(buf, len, &pos, "apicid       : ");
    // proc_put_uint(buf, len, &pos, apicid); // <--- Page Fault

    return pos;

    proc_puts(buf, len, &pos, "apicid       : ");
    proc_put_uint(buf, len, &pos, apicid);
    proc_putc(buf, len, &pos, '\n');

    /*
     * Flags
     */
    proc_puts(buf, len, &pos, "flags        : ");

    if (edx & (1 << 0))
        proc_puts(buf, len, &pos, "fpu ");

    if (edx & (1 << 4))
        proc_puts(buf, len, &pos, "tsc ");

    if (edx & (1 << 5))
        proc_puts(buf, len, &pos, "msr ");

    if (edx & (1 << 6))
        proc_puts(buf, len, &pos, "pae ");

    if (edx & (1 << 9))
        proc_puts(buf, len, &pos, "apic ");

    if (edx & (1 << 15))
        proc_puts(buf, len, &pos, "cmov ");

    if (edx & (1 << 23))
        proc_puts(buf, len, &pos, "mmx ");

    if (edx & (1 << 25))
        proc_puts(buf, len, &pos, "fxsr ");

    if (edx & (1 << 26))
        proc_puts(buf, len, &pos, "sse ");

    if (ecx & (1 << 0))
        proc_puts(buf, len, &pos, "sse3 ");

    if (ecx & (1 << 9))
        proc_puts(buf, len, &pos, "ssse3 ");

    if (ecx & (1 << 19))
        proc_puts(buf, len, &pos, "sse4_1 ");

    if (ecx & (1 << 20))
        proc_puts(buf, len, &pos, "sse4_2 ");

    if (ecx & (1 << 28))
        proc_puts(buf, len, &pos, "avx ");

    proc_putc(buf, len, &pos, '\n');

    /*
     * CPUID Level
     */
    proc_puts(buf, len, &pos, "cpuid level  : ");

    proc_put_uint(
        buf,
        len,
        &pos,
        proc_api->cpuid_max_leaf()
    );

    proc_putc(buf, len, &pos, '\n');

    /*
     * CPU MHz
     */
    uint64_t freq = proc_api->get_tsc_freq();

    proc_puts(buf, len, &pos, "cpu MHz      : ");

    proc_put_uint(
        buf,
        len,
        &pos,
        freq / 1000000
    );

    proc_puts(buf, len, &pos, ".000000\n");

    /*
     * Weitere Linux-artige Informationen
     */

    proc_putc(buf, len, &pos, '\n');

    return pos;
}

static size_t proc_task_status(char *buf, size_t len, struct task *task) {
    if (!buf || len == 0 || !task) {
        return 0;
    }

    size_t pos = 0;

    proc_puts(buf, len, &pos, "Name:    ");
    proc_puts(buf, len, &pos, task->name);
    proc_puts(buf, len, &pos, "\n");
    proc_puts(buf, len, &pos, "Umask:   ");
    proc_put_octal(buf, len, &pos, task->umask);
    proc_puts(buf, len, &pos, "\n");

    proc_puts(buf, len, &pos, "State:   ");

    switch (task->state) {
        case TASK_RUNNING: {
            proc_puts(buf, len, &pos, "R (running)");
            break;
        }

        case TASK_READY: {
            proc_puts(buf, len, &pos, "R (ready)");
            break;
        }

        case TASK_BLOCKED: {
            proc_puts(buf, len, &pos, "S (sleeping)");
            break;
        }

        default: {
            proc_puts(buf, len, &pos, "I (idle)");
            break;
        }
    }

    proc_puts(buf, len, &pos, "\n");
    proc_puts(buf, len, &pos, "Tgid:    ");
    proc_put_uint(buf, len, &pos, task->tgid);
    proc_puts(buf, len, &pos, "\n");

    proc_puts(buf, len, &pos, "Ngid:    ");
    proc_put_uint(buf, len, &pos, task->ngid);
    proc_puts(buf, len, &pos, "\n");

    proc_puts(buf, len, &pos, "Pid:     ");
    proc_put_uint(buf, len, &pos, task->id);
    proc_puts(buf, len, &pos, "\n");

    proc_puts(buf, len, &pos, "PPid:    ");
    proc_put_uint(buf, len, &pos, task->parent_id);
    proc_puts(buf, len, &pos, "\n");

    proc_puts(buf, len, &pos, "Uid:     ");
    proc_put_uint(buf, len, &pos, task->uid);
    proc_puts(buf, len, &pos, "    ");
    proc_put_uint(buf, len, &pos, task->uid);
    proc_puts(buf, len, &pos, "    ");
    proc_put_uint(buf, len, &pos, task->uid);
    proc_puts(buf, len, &pos, "    ");
    proc_put_uint(buf, len, &pos, task->uid);
    proc_puts(buf, len, &pos, "\n");

    proc_puts(buf, len, &pos, "Gid:     ");
    proc_put_uint(buf, len, &pos, task->gid);
    proc_puts(buf, len, &pos, "    ");
    proc_put_uint(buf, len, &pos, task->gid);
    proc_puts(buf, len, &pos, "    ");
    proc_put_uint(buf, len, &pos, task->gid);
    proc_puts(buf, len, &pos, "    ");
    proc_put_uint(buf, len, &pos, task->gid);
    proc_puts(buf, len, &pos, "\n");

    return pos;
}

static size_t proc_read(vfs_node_t* node, void* buf, size_t len) {
    if (!node || !buf || len == 0) {
        return 0;
    }

    proc_node_t* internal = (proc_node_t*)node->internal;

    if (!internal) {
        return 0;
    }

    switch (internal->type) {
        case PROC_UPTIME: {
            uint64_t now = proc_api->rdtsc();

            uint64_t freq = proc_api->get_tsc_freq();

            if (freq == 0) {
                return 0;
            }

            uint64_t ticks = now - boot_tsc;

            uint64_t seconds = ticks / freq;
            uint64_t remainder = ticks % freq;

            uint64_t centiseconds = (remainder * 100) / freq;

            return proc_format_uptime(buf, len, seconds, centiseconds);
        }

        case PROC_MEMINFO: {
            return proc_meminfo(buf, len);
        }

        case PROC_CPUINFO: {
            return proc_cpuinfo(buf, len);
        }

        case PROC_TASK_STATUS: {
            return proc_task_status((char*)buf, len, internal->task);
        }

        default: {
            return 0;
        }
    }
}

static size_t proc_write(vfs_node_t* node, const void* buf, size_t len) {
    (void)node;
    (void)buf;
    (void)len;

    return 0;
}

static void proc_delete(vfs_node_t* node) {
    (void)node;
}

static filesystem_t procfs = {
    .name = "proc",

    .mount = proc_mount,

    .create_file = proc_create_file,
    .create_dir = proc_create_dir,

    .read = proc_read,

    .write = proc_write,

    .delete = proc_delete
};

void proc_populate(vfs_node_t* root) {
    if (!root) {
        return;
    }

    proc_add_node(root, "uptime", false, PROC_UPTIME, NULL);
    proc_add_node(root, "meminfo", false, PROC_MEMINFO, NULL);
    proc_add_node(root, "cpuinfo", false, PROC_CPUINFO, NULL);

    proc_populate_tasks(root);
}

void proc_init(KernelAPI *api) {
    if (!api) {
        return;
    }

    proc_api = api;
    boot_tsc = api->rdtsc();

    api->vfs_register(&procfs);

    // Create /proc dir

    vfs_node_t* proc_dir = api->vfs_mkdir(api->vfs_get_root(), "proc");

    if (!proc_dir) {
        api->kprintf(WHITE "<" RED " ERROR " WHITE "> " "Failed to create /proc!\n");
        return;
    }

    vfs_node_t* proc = api->vfs_mount("/proc", "proc");
    proc_root = proc;

    if (!proc) {
        api->kprintf(WHITE "<" RED " ERROR " WHITE "> " "Failed to mount ProcFS!\n");
        return;
    }

    api->kprintf(WHITE "<" GREEN " KERNEL " WHITE "> " "ProcFS loaded!\n");

    proc_populate(proc);

    /*
    vfs_node_t* tmp = api->vfs_resolve_path("/proc");

    if (tmp) {
        api->vfs_ls(tmp);
    } */

    // api->kprintf("\n/proc/0/status: \n");

    vfs_node_t* node = api->vfs_resolve_path("/proc/cpuinfo");

    char buf[8192];

    size_t n = api->vfs_read(node, buf, sizeof(buf) - 1);

    buf[n] = '\0';
    
    // api->kprintf("%s", buf);


    // api->mouse_init();

    // asm volatile("int $44");

    // proc_test_uptime();

    // api->kprintf("after read\n");

    // buf[n] = '\0';

    // api->kprintf("result: %s", buf);

    /*
    api->kprintf("VFS Test: \n");

    if (tmp) {
        api->vfs_ls(tmp);
    } */
}