/*
 * CRC.c
 *
 *  Created on: 2025-12-22
 *      Author: Administrator
 */
#include "CRC.h"

unsigned short cal_crc_8005(unsigned char *byte, unsigned char nbyte){
	unsigned short itemp=0xFFFF;
	unsigned char i;
	while(nbyte--)
	{
		itemp ^= *byte;
		byte++;
		for (i=0; i<8; i++)
		{

			if (itemp & 0x1)
			{
				itemp >>= 1;
				itemp ^= 0xA001;

			}else
			{
				itemp >>= 1;

			}
		}
	}
	return itemp;
}

void crc_check_8005(uint8_t *byte, char *buf)
{
	sprintf(buf, "%04X", cal_crc_8005(byte, strlen((char *) byte)));
}

uint32_t cal_crc(uint8_t *byte, int nbyte)
{
	uint itemp = 0xFFFF;
	uint8_t i;
	while (nbyte--)
	{
		itemp ^= *byte++ << 8;
		for (i = 0; i < 8; i++)
		{
			if (itemp & 0x8000)
			{
				itemp <<= 1;
				itemp ^= 0x1021;
			}
			else
				itemp <<= 1;
		}
	}
	return itemp;
}

char *crc_check(uint8_t *byte, char *buf)
{
	char buf_tmp[20] = "";
	sprintf(buf_tmp, "%08X", cal_crc(byte, strlen((char *) byte)));
	strcpy(buf, buf_tmp + 4);
	return buf;
}

/*
 * 比较CRC,比较传入的带CRC的字符串的CRC是否相等
 * 参数：需要比较的字符串
 * 返回：0：相等，不为零：不相等
 */
int CompareCRC(char *buf)
{
	if(strlen(buf) < 7)
	{
		return -1;
	}
	uint8_t buf_tmp[4096] = "";
	char crc_buf_pc[5] = "";
	char crc_buf[16] = "";
	memset(&buf_tmp, 0, sizeof(buf_tmp));
	memcpy(buf_tmp, buf, strlen(buf) - 6);
	crc_check(buf_tmp, crc_buf);
	memset(&crc_buf_pc, 0, sizeof(crc_buf_pc));
	memcpy(crc_buf_pc, buf + strlen(buf) - 6, 4);
	return strcmp(crc_buf_pc, crc_buf);
}

/*
 * 比较CRC,比较传入的带CRC的字符串的CRC是否相等
 * 参数：需要比较的字符串
 * 返回：0：相等，不为零：不相等
 */
int CompareCRC_8005(char *buf)
{
	if(strlen(buf) < 7)
	{
		return -1;
	}
	uint8_t buf_tmp[4096] = "";
	char crc_buf_pc[5] = "";
	char crc_buf[16] = "";
	memset(&buf_tmp, 0, sizeof(buf_tmp));
	memcpy(buf_tmp, buf, strlen(buf) - 6);
	crc_check_8005(buf_tmp, crc_buf);
	memset(&crc_buf_pc, 0, sizeof(crc_buf_pc));
	memcpy(crc_buf_pc, buf + strlen(buf) - 6, 4);
	return strcmp(crc_buf_pc, crc_buf);
}

