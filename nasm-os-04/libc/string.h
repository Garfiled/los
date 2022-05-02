#ifndef STRINGS_H
#define STRINGS_H

void int_to_ascii(int n, char str[]);
void _itoa(int n, char str[], int radix);
void hex_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(char s[]);
void backspace(char s[]);
void append(char s[], char n);
int strcmp(char s1[], char s2[]);
int strcmpN(char s1[], char s2[],int n);
int atoi(char s[]);

#endif
