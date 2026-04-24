/*
 * TurnPlate_2_OP.h
 *
 *  Created on: 2026-1-5
 *      Author: Administrator
 */

#ifndef TURNPLATE_2_OP_H_
#define TURNPLATE_2_OP_H_

#include "Gloable_Schema.h"
#include "TurnPlate_2.h"
#include "RFID.h"
#include <sys/time.h>
#include <time.h>
#include <stdint.h>


#define RFID_3_ADDR		0x3F
#define TRACK_OFFSET_OFFSET 0x04
#define CAR_MAX_WRITE_TIME	95000

typedef char QElemtype;

typedef struct _node{
    QElemtype writeData[1000];
    int dataLen;
    uint8_t instrId[9];
    struct _node *next;
}Node;

/*用的时候记得初始化*/
typedef struct _list{
    struct _node * head;
    struct _node * tail;
}List;

List list;
List asyncCmdList;

char writeTraInfo(int UART_ADDR, int RFID_ADDR, message *_message, int pos, char *record);
char takeOutWriteData(List *list);
void packageNet(char *sendBuf);

void putInWriteData(List *list, char *writeData, int dataLen);
void Turntable_2_Sensor_3(int val);
void Turntable_2_Sensor_4(int val);

void InitList(List *list);
void ClearList(List *list);

extern int Turntable_2_trunNum;
extern int Turntable_2_lastLocatin;

#endif /* TURNPLATE_2_OP_H_ */
