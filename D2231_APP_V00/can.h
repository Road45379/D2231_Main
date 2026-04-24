/*
 * can.h
 *
 *  Created on: 2020-3-31
 *      Author: Dk
 */

#ifndef CAN_H_
#define CAN_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <stdint.h>
#include <sys/mman.h>

#define XCANPS_BTR_SJW_MASK	0x00000180U /**< Synchronization Jump Width */
#define XCANPS_BTR_SJW_SHIFT	7U
#define XCANPS_BTR_TS2_MASK	0x00000070U /**< Time Segment 2 */
#define XCANPS_BTR_TS2_SHIFT	4U
#define XCANPS_BTR_TS1_MASK	0x0000000FU /**< Time Segment 1 */

struct AXI_CAN_REG
{
	volatile int SRR;//00
	volatile int MSR;//04
	volatile int BRPR;//08
	volatile int BTR;//0C
	volatile int ECR;//10
	volatile int ESR;//14
	volatile int SR;//18
	volatile int ISR;//1C
	volatile int IER;//20
	volatile int ICR;//24
	volatile int Reserved1;//28
	volatile int Reserved2;//2C
	volatile int TXFIFO_ID;//30;
	volatile int TXFIFO_DLC;//34
	volatile int TXFIFO_DATA1;//38
	volatile int TXFIFO_DATA2;//3C
	volatile int TXHPB_ID;//40
	volatile int TXHPB_DLC;//44
	volatile int TXHPB_DATA1;//48
	volatile int TXHPB_DATA2;//4C
	volatile int RXFIFO_ID;//50
	volatile int RXFIFO_DLC;//54
	volatile int RXFIFO_DATA1;//58
	volatile int RXFIFO_DATA2;//5C
	volatile int AFR;//60
	volatile int AFMR1;//64
	volatile int AFIR1;//68
	volatile int AFMR2;//6C
	volatile int AFIR2;//70
	volatile int AFMR3;//74
	volatile int AFIR3;//78
	volatile int AFMR4;//7C
	volatile int AFIR4;//80
};

typedef enum
{
	can_1,
	can_2,
	can_3,
	can_4,
	can_5,
	can_6,
	can_7,
	can_8,
	can_9,
}CAN_NUM;

typedef enum{
	CAN_BIT_10K,
	CAN_BIT_20K,
	CAN_BIT_40K,
	CAN_BIT_50K,
	CAN_BIT_80K,
	CAN_BIT_100K,
	CAN_BIT_125K,
	CAN_BIT_200K,
	CAN_BIT_250K,
	CAN_BIT_400K,
	CAN_BIT_500K,
	CAN_BIT_800K,
	CAN_BIT_1000K
}BitConfigBit;

typedef struct
{
	uint32_t Data1;
	uint32_t Data2;
}Data;

typedef struct
{
	char IsItemUsed;
	unsigned int PhyAddress;
	int _IRQ;
	//void (*callback_CanRsv)(unsigned char, int);//回调函数
	int calbckpra;//回调附加数据
	int InterruptAttachId;
	struct sigevent _sigevent;
	volatile struct AXI_CAN_REG *m_AXI_CAN_REG;
	void(*func_call)(int ID,int DLC,int Dat1,int Dat2);
} DevCan;

int Can_Open(unsigned int Can_num, int BitRate,void(*func_rcv)(
		int,int,int,int), int usrdat);
int Can_Write(uint32_t fd, uint8_t IsExtendedID,uint32_t ID, uint8_t IsRemoteFrames,uint32_t DLC, uint32_t Dat1, uint32_t Dat2);

uint32_t MergeID(int dstaadr, int motoraddr,int cmdnum);
Data MergeData(unsigned int tmp[8]);

int ToTXFIFO_ID(int ID);

#endif /* CAN_H_ */
