/*
 * ASCII_Transition.h
 *
 *  Created on: 2025-12-23
 *      Author: ThinkPad
 */

#ifndef ASCII_TRANSITION_H_
#define ASCII_TRANSITION_H_
#include "Gloable_Schema.h"

int tolower(int c);
int htoi(char s[]);
unsigned char AsciiToHex(char c);
unsigned char PackByte(char* p);
unsigned short PackWord(char* p);
unsigned int Pack8Byte(char* p);
char *crc_check(uint8_t *byte, char *buf);

//void my_printf(char *format, ...);

#endif /* ASCII_TRANSITION_H_ */
