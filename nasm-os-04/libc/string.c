#include "string.h"
#include <stdint.h>

/**
 * K&R implementation
 */

void int_to_ascii(int n, char str[])
{
  _itoa(n, str, 10);
}

void _itoa(int n, char str[], int radix) {
    int i, sign;
    if ((sign = n) < 0) n = -n;
    i = 0;
    do {
        int aa = n % radix;
        if (aa <= 9) {
          str[i++] = aa + '0';
        } else {
          str[i++] = aa - 10 + 'A';
        }
    } while ((n /= radix) > 0);

    if (sign < 0) str[i++] = '-'; 
    str[i] = '\0';

    reverse(str);
}

void hex_to_ascii(int n, char str[]) {
    append(str, '0');
    append(str, 'x');
    char zeros = 0;

    int32_t tmp;
    int i;
    for (i = 28; i > 0; i -= 4) {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && zeros == 0) continue;
        zeros = 1;
        if (tmp > 0xA) append(str, tmp - 0xA + 'a');
        else append(str, tmp + '0');
    }

    tmp = n & 0xF;
    if (tmp >= 0xA) append(str, tmp - 0xA + 'a');
    else append(str, tmp + '0');
}

/* K&R */
void reverse(char s[]) {
    int c, i, j;
    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* K&R */
int strlen(char s[]) {
    int i = 0;
    while (s[i] != '\0') ++i;
    return i;
}

void append(char s[], char n) {
    int len = strlen(s);
    s[len] = n;
    s[len+1] = '\0';
}

void backspace(char s[]) {
    int len = strlen(s);
    s[len-1] = '\0';
}

/* K&R 
 * Returns <0 if s1<s2, 0 if s1==s2, >0 if s1>s2 */
int strcmp(char s1[], char s2[]) {
    int i;
    for (i = 0; s1[i] == s2[i]; i++) {
        if (s1[i] == '\0') return 0;
    }
    return s1[i] - s2[i];
}

int strcmpN(char s1[], char s2[],int n) {
    for (int i = 0;i<n; i++) {
		if (s1[i]==s2[i])
			continue;
		else
			return s2[i]-s1[i];
    }
    return 0;
}

int atoi(char* str) 
{ 
    int res = 0; // Initialize result 
    int sign = 1; // Initialize sign as positive 
    int i = 0; // Initialize index of first digit 
    int dec = 10;

    if (str[0]=='0' && str[1]=='x')
    {
        str += 2;
        dec = 16;
    }
  
    // If number is negative, then update sign 
    if (str[0] == '-') { 
        sign = -1; 
        i++; // Also update index of first digit 
    } 
  
    // Iterate through all digits and update the result 
    for (; str[i] != '\0'; ++i) {
        char vv;
        if (str[i]>='a')
            vv = str[i] - 'a' + 10;
        else if (str[i]>='A')
            vv = str[i] - 'A'+10;
        else
            vv = str[i] - '0';
        res = res * dec + vv; 
  
    }

    // Return result with sign 
    return sign * res; 
}
