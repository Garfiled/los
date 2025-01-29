#pragma once

#include <stddef.h>

void int_to_ascii(int n, char *str);
void _itoa(int n, char *str, int radix);
void hex_to_ascii(int n, char *str);
void reverse(char *s);
int strlen(char *s);
void backspace(char *s);
void append(char *s, char c);
int strcmp(char *s1, char *s2);
int strcmpN(char *s1, char *s2, int n);
int atoi(char *s);
void memmove(char *s1, char *s2, int n);
void memset(char *s, char c, int n);
void* memcpy(void *dst, const void *src, size_t count);

#define STRCMP(a, b) strcmp((char*)a, (char*)b)
#define STRCMPN(a, b, n) strcmpN((char*)a, (char*)b, n)
#define MEMMOVE(a, b, n) memmove((char*)a, (char*)b, n)
#define MEMSET(s, c, n) memset((char *)s, c, n)
