#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 10
#define MAX_NAME_LEN 32
#define MAX_CONTENT_LEN 256

typedef struct {
    char name[MAX_NAME_LEN];
    char content[MAX_CONTENT_LEN];
    int used;
} RamFile;

typedef struct {
    RamFile files[MAX_FILES];
} RamFs;

// Datei anlegen
void create_file(RamFs *fs, const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs->files[i].used) {
            strncpy(fs->files[i].name, name, MAX_NAME_LEN);
            fs->files[i].content[0] = '\0';
            fs->files[i].used = 1;
            printf("File '%s' created.\n", name);
            return;
        }
    }
    printf("No space left in RAMFS!\n");
}

// Datei schreiben
void write_file(RamFs *fs, const char *name, const char *data) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used && strcmp(fs->files[i].name, name) == 0) {
            strncpy(fs->files[i].content, data, MAX_CONTENT_LEN);
            printf("Written to '%s'.\n", name);
            return;
        }
    }
    printf("File '%s' not found.\n", name);
}

// Datei lesen
void read_file(RamFs *fs, const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used && strcmp(fs->files[i].name, name) == 0) {
            printf("Content of '%s': %s\n", name, fs->files[i].content);
            return;
        }
    }
    printf("File '%s' not found.\n", name);
}

// Dateien auflisten
void list_files(RamFs *fs) {
    printf("Files in RAMFS:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs->files[i].used) {
            printf(" - %s\n", fs->files[i].name);
        }
    }
}

int main() {
    RamFs fs = {0};

    create_file(&fs, "test.txt");
    write_file(&fs, "test.txt", "Hallo RAMFS!");
    read_file(&fs, "test.txt");
    list_files(&fs);

    return 0;
}
