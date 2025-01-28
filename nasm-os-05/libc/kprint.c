#include "libc/string.h"
#include "libc/kprint.h"
#include <stdarg.h>

void putchar(char c)
{
  kprint_char(c);
}

int puts(char *str)
{
  return kprint(str);
}

int kprintf(char *fmt, ...)
{
	//显示数字缓冲区
	char buff[0x800];
	//显示字符串指针
	char *str;
	//显示字符变量
	char ch;
	//显示字符总数
	int count = 0;

	//动态参数
	va_list args;
	//初始化动态参数
	va_start(args, fmt);

	//读到\0为结束
	while (*fmt != '\0') {
		//格式化标记%
		if (*fmt == '%') {
			//显示一个字符
			if ('c' == *(fmt + 1)) {
				ch = va_arg(args, char);
				putchar(ch);
				count++;
				fmt += 2;
			}
			//显示字符串
			else if ('s' == *(fmt + 1)) {
				str = va_arg(args, char*);
				count += puts(str);
				fmt += 2;
			}
			//显示整数
			else if ('d' == *(fmt + 1)) {
				_itoa(va_arg(args, int), buff, 10);
				count += puts(buff);
				fmt += 2;
			}
			//显示无符号16进制整数
			else if ('x' == *(fmt + 1)) {
				unsigned int num = va_arg(args, unsigned int);
				unsigned int nl = num & 0xffff;
				unsigned int nh = (num >> 16) & 0xffff;
				count += puts("0x");
				if (nh == 0) {
					_itoa(nl, buff, 16);
					count += puts(buff);
				} else {
          _itoa(nh, buff, 16);
					count += puts(buff);

          _itoa(nl, buff, 16);
					int zero = 4 - strlen(buff);
					for (int i = 0; i < zero; i++)
					{
						putchar('0');
					}
					count += puts(buff);
				}
				fmt += 2;
			}
		}
		//显示普通字符
		else {
			putchar(*fmt++);
			count++;
		}
	}
	return count;
}
