#pragma once

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f
#define RED_ON_WHITE 0xf4

/* Screen i/o ports */
#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

/* Public kernel API */
void clear_screen();
int kprint_at(char *message, int col, int row);
void kprint_k(const char *message, int k);
int kprint(char *message);
void kprint_backspace();
void kprint_int(int val);
void kprint_hex(char *message);
void kprint_hex_n(char *message, int n);
void kprint_char(char c);
