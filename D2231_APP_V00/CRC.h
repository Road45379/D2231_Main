/*
 * CRC.h
 *
 *  Created on: 2025-12-22
 *      Author: Administrator
 */
#include "Gloable_Schema.h"

void crc_check_8005(uint8_t *byte, char *buf);
int CompareCRC(char *buf);
int CompareCRC_8005(char *buf);
