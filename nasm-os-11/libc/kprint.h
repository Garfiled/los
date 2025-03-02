#pragma once

enum LOG_LEVEL
{
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4
};

extern enum LOG_LEVEL default_log_level;

int kprintf(const char *fmt, ...);
#define LOGD(format, ...) {if (DEBUG >= default_log_level) { kprintf("[DEBUG] [%s:%d] " format,__FILE_NAME__,__LINE__,##__VA_ARGS__);}}
#define LOGI(format, ...) {if (INFO >= default_log_level) { kprintf("[INFO] [%s:%d] " format,__FILE_NAME__,__LINE__,##__VA_ARGS__);}}
#define LOGW(format, ...) {if (WARN >= default_log_level) { kprintf("[WARN] [%s:%d] " format,__FILE_NAME__,__LINE__,##__VA_ARGS__);}}
#define LOGE(format, ...) {if (ERROR >= default_log_level) { kprintf("[ERROR] [%s:%d] " format,__FILE_NAME__,__LINE__,##__VA_ARGS__);}}
