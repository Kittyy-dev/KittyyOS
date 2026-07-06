#include <fat.h>
#include <vga.h>
#include <string.h>

void fat32_test_write(void) {
    const char* text = "Hallo KittyyOS!\r\n";
    uint32_t len = strlen(text);

    // Datei öffnen/erzeugen – 8.3 Name: "TEST    TXT"
    FileHandle fh = open_file("TEST    TXT", &bpb, fat_ram, dir_buf);

    int res = write_file(fh, &bpb, fat_ram, (const void*)text, len);
    if (res != 0) {
        vga_printf("write_file failed: %d\n", res);
        return;
    }

    vga_print("TEST.TXT geschrieben.\n");
}

void fat32_test_read(void) {
    FileHandle fh = open_file("TEST    TXT", &bpb, fat_ram, dir_buf);

    static uint8_t buffer[4096]; // genug Platz
    int res = read_file(fh, &bpb, fat_ram, buffer);
    if (res != 0) {
        vga_printf("read_file failed: %d\n", res);
        return;
    }

    vga_printf("Inhalt von TEST.TXT:\n%s\n", buffer);
}
