/*
 * can.c
 *
 *  Created on: 2020-3-31
 *      Author: Dk
 */
#include "can.h"

static DevCan _DevCan[9];
unsigned int CANPhyAddr;

#define freqCAN_REF_CLK 20000000

typedef struct
{
	int bit;
	int BRPR;
	int TS1;
	int TS2;
}BitConfig;

static BitConfig _BitConfig[]=
{
		/*CAN时钟20M*/
		/*{10, 124, 7, 6},
		{20, 99, 4, 3},
		{40, 49, 4, 3},
		{50, 24, 7, 6},
		{80, 24, 4, 3},
		{100, 24, 2, 3},
		{125, 15, 4, 3},
		{200, 9, 4, 3},
		{250, 7, 4, 3},
		{400, 4, 4, 3},
		{500, 7, 1, 1},
		{800, 4, 1, 1},
		{1000, 3, 1, 1}*/

		//CAN时钟23.809500M
		/*{10,149,7,6},
		{20,74,7,6},
		{40,49,5,4},
		{50,29,7,6},
		{80,24,5,4},
		{100,14,7,6},
		{125,11,7,6},
		{200,14,3,2},
		{250,5,7,6},
		{400,4,5,4},
		{500,2,7,6},
		{800,2,4,3},
		{1000,2,3,2}*/

		/*CAN时钟100M*/
		{0, 0, 0, 0},//没用，但是不能删
		{20, 249, 11, 6},
		{40, 124, 11, 6},
		{50, 124, 7, 6},
		{80, 124, 3, 4},
		{100, 99, 4, 3},
		{125, 49, 7, 6},
		{200, 49, 4, 3},
		{250, 24, 7, 6},
		{400, 24, 4, 3},
		{500, 24, 2, 3},
		{800, 24, 1, 1},
		{1000, 9, 4, 3}

};

typedef struct
{
	int Addr;
	int IRQ;
}CAN_NUM_TO_ADDR;

static  CAN_NUM_TO_ADDR _CAN_NUM_TO_ADDR[] =
{
		{0x43C50000,67},
		{0x43C60000,68},
		{0x43C70000,84},
		{0x43C80000,85},
		{0x43C90000,86},
		{0x43CA0000,87},
		{0x43CC0000,88},
		{0x43CD0000,89},
		{0x43CE0000,90}
};

static const struct sigevent *_isr_handler(void *arg, int id)
{
	int fd = (int) arg;
	InterruptMask(_DevCan[fd]._IRQ, _DevCan[fd].InterruptAttachId);
	return (&_DevCan[fd]._sigevent);
}

void CanIntHandle(int fd)
{
	if (_DevCan[fd].m_AXI_CAN_REG->ISR & 0x80)
	{
		if (_DevCan[fd].func_call != NULL)
		{
			_DevCan[fd].func_call(_DevCan[fd].m_AXI_CAN_REG->RXFIFO_ID,
					_DevCan[fd].m_AXI_CAN_REG->RXFIFO_DLC,
					_DevCan[fd].m_AXI_CAN_REG->RXFIFO_DATA2,
					_DevCan[fd].m_AXI_CAN_REG->RXFIFO_DATA1);
		}
		_DevCan[fd].m_AXI_CAN_REG->ICR|=0x80;//Clear Receive FIFO Not Empty Interrupt
	}
	InterruptUnmask(_DevCan[fd]._IRQ, _DevCan[fd].InterruptAttachId);
}

void* Can_int_thread(void *arg)
{
	int fd = (int) arg;
	SIGEV_INTR_INIT(&(_DevCan[fd]._sigevent));
	_DevCan[fd].InterruptAttachId = InterruptAttach(_DevCan[fd]._IRQ,
				_isr_handler, arg, 0, _NTO_INTR_FLAGS_TRK_MSK);
	if (_DevCan[fd].InterruptAttachId == -1)
	{
		printf("ISR attach Error!\n");
	}
	while (1)
	{
		InterruptWait(0, NULL);
		CanIntHandle(fd);
		InterruptUnmask(_DevCan[fd]._IRQ, _DevCan[fd].InterruptAttachId);
	}
}

int Can_Open(unsigned int Can_num, int BitRate,void(*func_rcv)(
		int,int,int,int), int usrdat)
{
	if (ThreadCtl(_NTO_TCTL_IO, 0) == -1)
			printf("WRONG PERMISSION\n");
	/*if(Can_num == 0){
		CANPhyAddr = 0xE0008000;
	}else if(Can_num == 1){
		CANPhyAddr = 0xE0009000;
	}*/
	CANPhyAddr = _CAN_NUM_TO_ADDR[Can_num].Addr;
	int fd = -1;
	int i;
	for(i = 0; i < sizeof(_DevCan)/sizeof(DevCan);i++){
		if(_DevCan[i].PhyAddress == CANPhyAddr){
			return -2;
		}
	}
	for(i = 0; i < sizeof(_DevCan)/sizeof(DevCan);i++){
		if(_DevCan[i].IsItemUsed != 0x55){
			fd = i;
			break;
		}
	}
	if(fd == -1){
		return -2;
	}
	_DevCan[fd].m_AXI_CAN_REG = (struct AXI_CAN_REG *)mmap_device_io(0x1000, CANPhyAddr);
	if(_DevCan[fd].m_AXI_CAN_REG == 0){
		return -3;
	}
	_DevCan[fd].func_call = func_rcv;
	_DevCan[fd].calbckpra = usrdat;
	_DevCan[fd]._IRQ = _CAN_NUM_TO_ADDR[Can_num].IRQ;
	_DevCan[fd].IsItemUsed = 0x55;

	//fBIT_RATE = freqCAN_REF_CLK / (BRP + 1) * (3 + TS1 + TS2))
	int BRPR = _BitConfig[BitRate].BRPR;
	_DevCan[fd].m_AXI_CAN_REG->BRPR = BRPR;
	int TS1 = _BitConfig[BitRate].TS1;
	int TS2 = _BitConfig[BitRate].TS2;

	_DevCan[fd].m_AXI_CAN_REG->BTR = (TS1) & XCANPS_BTR_TS1_MASK;
	_DevCan[fd].m_AXI_CAN_REG->BTR |= ((TS2) << XCANPS_BTR_TS2_SHIFT) & XCANPS_BTR_TS2_MASK;
	_DevCan[fd].m_AXI_CAN_REG->BTR |= (3 << XCANPS_BTR_SJW_SHIFT) & XCANPS_BTR_SJW_MASK;

	_DevCan[fd].m_AXI_CAN_REG->IER |= 0x80;//RX FIFO Not Empty Intr Mask
	//Enter Nomal Mode
	_DevCan[fd].m_AXI_CAN_REG->MSR = 0;
	_DevCan[fd].m_AXI_CAN_REG->SRR = 2;


	pthread_create(NULL, NULL, Can_int_thread, (void*) fd);
	return fd;
}

int Can_Write(uint32_t fd, uint8_t IsExtendedID,uint32_t ID, uint8_t IsRemoteFrames,uint32_t DLC, uint32_t Dat1, uint32_t Dat2)
{
	if(fd >= sizeof(_DevCan)/sizeof(DevCan))
	{
		return -1;
	}
	if(_DevCan[fd].IsItemUsed != 0x55){
		return -2;
	}
	uint32_t _ID_Reg_ = 0;
	if(IsExtendedID)
	{
		ID = ID & 0x1FFFFFFF;
		_ID_Reg_ = (ID & 0x1FFC0000) << 3;
		_ID_Reg_ |= (ID &0x3FFFF) << 1;
		_ID_Reg_ |= (0x1 << 19);
		if(IsRemoteFrames)
		{
			_ID_Reg_ |= 0x1;
		}
	}else
	{
		ID = ID & 0x7FF;
		_ID_Reg_ = ID << 21;
		if(IsRemoteFrames)
		{
			_ID_Reg_ |= (0x1 << 20);
		}
	}
	_DevCan[fd].m_AXI_CAN_REG->TXFIFO_ID = _ID_Reg_;
	_DevCan[fd].m_AXI_CAN_REG->TXFIFO_DLC = DLC << 28;
	_DevCan[fd].m_AXI_CAN_REG->TXFIFO_DATA1 = Dat1;
	_DevCan[fd].m_AXI_CAN_REG->TXFIFO_DATA2 = Dat2;
	return 0;
}

uint32_t MergeID(int dstaadr, int motoraddr, int cmdnum)
{
	uint32_t tmp_ID = 0;
	tmp_ID = (dstaadr << 20) | (motoraddr << 16) |  cmdnum ;
	return tmp_ID;
}



Data MergeData(unsigned int tmp[8])
{
	Data tmp_Data;
	tmp_Data.Data1 = (tmp[0] << 24) | (tmp[1] << 16) | (tmp[2] << 8) | (tmp[3]);
	tmp_Data.Data2 = (tmp[4] << 24) | (tmp[5] << 16) | (tmp[6] << 8) | (tmp[7]);
	return tmp_Data;
}
