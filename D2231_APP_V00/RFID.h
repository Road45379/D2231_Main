/*
 * RFID.h
 *
 *  Created on: 2026-1-4
 *      Author: ThinkPad
 */

#ifndef RFID_H_
#define RFID_H_
#include "Gloable_Schema.h"

#define RFID_1_ADDR     0x0F//卸载点RFID
#define RFID_2_ADDR     0x1F//加样点RFID



int Read_RFID_Barcode(int RFID_ADDR);//读轨迹信息的偏移扇区，固定从偏移0x04扇区读
int Write_RFID_Barcode(int RFID_ADDR, char *BarCode, int len);//写小车存储的条码
int Read_RFID(int UART_ADDR, int RFID_ADDR, int offset);
int Read_RFID_Barcode2(int UART_ADDR, int RFID_ADDR, message *_message);
int Write_RFID(int UART_ADDR, int RFID_ADDR, int offset, char * buf);

int clear_car_message();
int clear_car_tarck_or_message(int RFID_ADDR, uint8_t flag, uint32_t car_id);

#endif /* RFID_H_ */
