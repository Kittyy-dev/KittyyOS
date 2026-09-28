#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int memcmp(const void* s1, const void* s2, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* dest, int c, size_t n);

int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, int n);
int strlen(const char *s);

char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);

int strcasecmp(const char* a, const char* b);
char *strtok(char* str, const char* delimiters);
char *strncpy(char *dest, const char *src, size_t n);

#endif
