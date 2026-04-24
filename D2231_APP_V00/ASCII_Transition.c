/*
 * ASCII_Transition.c
 *
 * ASCII×ª»»
 *
 *  Created on: 2025-12-23
 *      Author: ThinkPad
 */

#include <stdarg.h>

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 'a' - 'A';
    }
    else
    {
        return c;
    }
}

int htoi(char s[])
{
    int i;
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))
    {
        i = 2;
    }
    else
    {
        i = 0;
    }
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)
    {
        if (tolower(s[i]) > '9')
        {
            n = 16 * n + (10 + tolower(s[i]) - 'a');
        }
        else
        {
            n = 16 * n + (tolower(s[i]) - '0');
        }
    }
    return n;
}


unsigned char AsciiToHex(char c)
{
	if ('0' <= c && c <= '9')
		return c-'0';
	else if ('A' <= c && c <= 'F')
		return c-'A'+0x0A;
	else if ('a' <= c && c <= 'f')
		return c-'a'+0x0A;
	else
	{
		return 0;
	}
}

unsigned char PackByte(char* p)
{
	return (AsciiToHex(p[0]) << 4) | AsciiToHex(p[1]);
}

unsigned short PackWord(char* p)
{
	return ((((unsigned short)PackByte(p)) << 8) | PackByte(p+2));
}

unsigned int Pack8Byte(char* p)
{
	return ((((unsigned short)PackWord(p)) << 16) | PackWord(p+4));
	/*return (AsciiToHex(p[0]) << 28) | (AsciiToHex(p[1]) << 24) | (AsciiToHex(p[2]) << 20)
		| (AsciiToHex(p[3]) << 16) | (AsciiToHex(p[4]) << 12) | (AsciiToHex(p[5]) << 8)
		| (AsciiToHex(p[6]) << 4) | (AsciiToHex(p[7]));*/
}
//#if 1
//#define Debug_Printf
//#endif
//void my_printf(char *format, ...)
//{
//#ifdef Debug_Printf
//	va_list args;
//	va_start(args, format);
//	vprintf(format, args);
//	va_end(args);
//#endif
//}
