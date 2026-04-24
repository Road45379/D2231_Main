/*
 * CAN_User_Code.c
 *
 *  Created on: 2026-1-5
 *      Author: ThinkPad
 */

#include "Gloable_Schema.h"
#include "can.h"
volatile int fd_can_800K;



static queue *PQueue_RecvCAN = NULL;//can接受队列

void can_func_call(int ID,int DLC,int Dat1,int Dat2)
{
	int _ID = (0 >> 21);
	int _DLC = (DLC >> 28);
	if (ID & (0x1 << 19))
	{
		_ID = ((ID & 0xFFE00000) >> 3 | ((ID & 0x7FFFE) >> 1));
		_ID |= 0x80000000;
		if (ID & 0x1)
		{
			_ID |= 0x40000000;
		}
	}
	else
	{
		_ID = (ID >> 21) & 0x7FF;
		if (ID & 0x100000)
		{
			_ID |= 0x40000000;
		}
	}
	char recv_queue_buf_tmp[40];
	sprintf(recv_queue_buf_tmp, ">%08X,%08X,%08X,%08X\r\n",
			_ID, _DLC & 0xF, Dat1, Dat2);
	printf("can recv : %s", recv_queue_buf_tmp);
	//数据存入接收队列
	enQueue(PQueue_RecvCAN, recv_queue_buf_tmp, strlen(recv_queue_buf_tmp));
 }

recv_mgstrt _recv_mgstrt;//接收数据解析后存结构体

void Recv_CanToCom(char *buff)
{
	char cmd_buf[16] = "";
	//char numLength_buf[16] = "";
	char data_buf_H[32] = "";

	memcpy(cmd_buf, buff + 1, 8);//从">"后开始截取帧ID
	//memcpy(numLength_buf, buff + 10, 8);//数据长度
	strncat(data_buf_H, buff + 19, 8);//数据
	strncat(data_buf_H, buff + 28, 8);

	unsigned int cmd_tmp = 0;
	sscanf(cmd_buf, "%08X", &cmd_tmp);//转为十六进制输

	memset(_recv_mgstrt.Data_H, 0, sizeof(_recv_mgstrt.Data_H));
	_recv_mgstrt.srcaddr = (cmd_tmp & 0xF000) >> 12;
	_recv_mgstrt.motoraddr = (cmd_tmp & 0xF00) >> 8;
	_recv_mgstrt.autoresponse = (cmd_tmp & 0x80) >> 7;
	_recv_mgstrt.cmdnum = (cmd_tmp & 0x7F);//功能码
	strcpy((char*)_recv_mgstrt.Data_H,data_buf_H);
	//sscanf(numLength_buf, "%X", &_recv_mgstrt.DLC);//长度
}


/*
 * 分控板接收接收解析
 */
static char cmdtmp[64];
static int n = 0;
void ControlBoardRecvAnalysis(char* buff, void (*ControlBoardRecv)(recv_mgstrt*))
{
	int ret = 0;
	int i;
	ret = strlen(buff);
	if(ret > 0)
	{
		for(i = 0; i < ret; i++)
		{
			char t = buff[i];
			if(t == '>')//判断帧头
			{
				n = 0;
			}
			cmdtmp[n++] = t;
			if(t == 0x0A)//判断帧尾
			{
				if(n == 38)
				{
					//printf("buff = %s", buff);
					Recv_CanToCom(buff);
					//sprintf(recv_queue_buf_tmp,"%02X%02X\n",_recv_mgstrt.srcaddr, _recv_mgstrt.cmdnum);//解析后的源地址和功能码，存入队列
					ControlBoardRecv(&_recv_mgstrt);
				}
			}
		}

	}
}

/*
 * 读版本返回
 */
void ReadVersion_Recv(recv_mgstrt* _recv_mgstrt)
{
	memset(_ControlBoard[_recv_mgstrt->srcaddr + 4].Motor[_recv_mgstrt->motoraddr].point, 0, sizeof(_ControlBoard[_recv_mgstrt->srcaddr + 4].Motor[_recv_mgstrt->motoraddr].point));
	memcpy(_ControlBoard[_recv_mgstrt->srcaddr + 4].Motor[_recv_mgstrt->motoraddr].point, _recv_mgstrt->Data_H, 10);
	_ControlBoard[_recv_mgstrt->srcaddr + 4].Motor[_recv_mgstrt->motoraddr].Send_OK[_recv_mgstrt->cmdnum] = true;
}

/*
 * 打开或关闭扫码枪
 */
void OpenOrCloseScan_Recv(recv_mgstrt* _recv_mgstrt)
{
	_ControlBoard[_recv_mgstrt->srcaddr + 4].Motor[_recv_mgstrt->motoraddr].State[_recv_mgstrt->cmdnum] = PackByte( _recv_mgstrt->Data_H);
	_ControlBoard[_recv_mgstrt->srcaddr + 4].Motor[_recv_mgstrt->motoraddr].Send_OK[_recv_mgstrt->cmdnum] = true;
}

/*
 * 打开或关闭扫码枪
 */
void OpenOrCloseScan_2_Recv(recv_mgstrt* _recv_mgstrt)
{
	_ControlBoard[_recv_mgstrt->srcaddr + 4].Motor[_recv_mgstrt->motoraddr].State[_recv_mgstrt->cmdnum] = PackByte( _recv_mgstrt->Data_H);
	_ControlBoard[_recv_mgstrt->srcaddr + 4].Motor[_recv_mgstrt->motoraddr].Send_OK[_recv_mgstrt->cmdnum] = true;
}


/*
 * 扫码返回
 */
char SampleShelf_tmp_Scan[33];
int SampleShelf_tmp_Scan_indexs = 0;

int code_tmp[4] = {0x01, 0x0A, 0x0B, 0x09};

void Scan_Recv(recv_mgstrt* _recv_mgstrt)
{
	int len1 = PackByte(_recv_mgstrt->Data_H);
	int num = PackByte(_recv_mgstrt->Data_H + 2);
	if(num == 1)//第一包、清缓冲区
	{
		memset(SampleShelf_tmp_Scan, 0, sizeof(SampleShelf_tmp_Scan));
		SampleShelf_tmp_Scan_indexs = 0;
	}
	if(((len1 + 5) / 6) == num)//判断是否为最后一包
	{
		int len2 = (len1 - ((num - 1) * 6)) * 2;//计算最后一包长度
		memcpy(SampleShelf_tmp_Scan + SampleShelf_tmp_Scan_indexs, _recv_mgstrt->Data_H + 4, len2);
		SampleShelf_tmp_Scan_indexs += len2;
		NetCmd *cmd_tmp = NULL;
		cmd_tmp = (NetCmd *) malloc(sizeof(NetCmd));
		memset(cmd_tmp->netcmdhead, 0, sizeof(cmd_tmp->netcmdhead));
		strcpy(cmd_tmp->netcmdhead, ">FFFFFFFF0202");
		cmd_tmp->code = 0x84;
		char s1[9] = {'0', '0', '0', '0', '0', '0', '0', '0', '\0'};
		int i = 0;
		int n = 0;
		char send_buf[32];
		sprintf(s1, "%0X%07X", Get_ManipulatorModule_Area(), len1);
		for(i = 0; i < SampleShelf_tmp_Scan_indexs;)
		{
			send_buf[n] = PackByte(SampleShelf_tmp_Scan + i);
			i += 2;
			n++;
		}
		send_buf[n] = '\0';
		//FinishRecv(cmd_tmp, 0, 0xFF, 5, 0x0000, s1, s2, s3, s4);
		Eth_Send_Queue(cmd_tmp, 0, 0xFF, 3, 0x0000, s1, send_buf);
		free((void *)cmd_tmp);
		cmd_tmp = NULL;
		BarCode_recv_finish_1 = 1;
		memset(SampleShelf_tmp_Scan, 0, sizeof(SampleShelf_tmp_Scan));
		SampleShelf_tmp_Scan_indexs = 0;
		SampleShelf_Scan_Succeed = 1;
	}
	else
	{
		memcpy(SampleShelf_tmp_Scan + SampleShelf_tmp_Scan_indexs, _recv_mgstrt->Data_H + 4, 12);
		SampleShelf_tmp_Scan_indexs += 12;
	}
}

/*
 * 扫码返回
 */
char TurnPlate_1_tmp_Scan[33];
int TurnPlate_1_tmp_Scan_indexs = 0;


void Scan_Recv_2(recv_mgstrt* _recv_mgstrt)
{
	int len1 = PackByte(_recv_mgstrt->Data_H);
	int num = PackByte(_recv_mgstrt->Data_H + 2);
	if(num == 1)//第一包、清缓冲区
	{
		memset(TurnPlate_1_tmp_Scan, 0, sizeof(TurnPlate_1_tmp_Scan));
		TurnPlate_1_tmp_Scan_indexs = 0;
	}
	if(((len1 + 5) / 6) == num)//判断是否为最后一包
	{
		int len2 = (len1 - ((num - 1) * 6)) * 2;//计算最后一包长度
		memcpy(TurnPlate_1_tmp_Scan + TurnPlate_1_tmp_Scan_indexs, _recv_mgstrt->Data_H + 4, len2);
		TurnPlate_1_tmp_Scan_indexs += len2;
		NetCmd *cmd_tmp = NULL;
		cmd_tmp = (NetCmd *) malloc(sizeof(NetCmd));
		memset(cmd_tmp->netcmdhead, 0, sizeof(cmd_tmp->netcmdhead));
		strcpy(cmd_tmp->netcmdhead, ">FFFFFFFF0402");
		cmd_tmp->code = 0x84;
		char s1[9] = {'0', '0', '0', '0', '0', '0', '0', '0', '\0'};
		int i = 0;
		int n = 0;
		char send_buf[32];
		sprintf(s1, "%08X", len1);
		for(i = 0; i < TurnPlate_1_tmp_Scan_indexs;)
		{
			send_buf[n] = PackByte(TurnPlate_1_tmp_Scan + i);
			i += 2;
			n++;
		}
		send_buf[n] = '\0';
		//FinishRecv(cmd_tmp, 0, 0xFF, 5, 0x0000, s1, s2, s3, s4);
		Eth_Send_Queue(cmd_tmp, 0, 0xFF, 3, 0x0000, s1, send_buf);
		free((void *)cmd_tmp);
		cmd_tmp = NULL;
		BarCode_recv_finish_2 = 1;
		memset(TurnPlate_1_tmp_Scan, 0, sizeof(TurnPlate_1_tmp_Scan));
		TurnPlate_1_tmp_Scan_indexs = 0;
		SampleShelf_Scan_Succeed = 1;
	}
	else
	{
		memcpy(TurnPlate_1_tmp_Scan + TurnPlate_1_tmp_Scan_indexs, _recv_mgstrt->Data_H + 4, 12);
		TurnPlate_1_tmp_Scan_indexs += 12;
	}
}

/*
 * 开蜂鸣器返回
 */
void BuzzerOpen_Recv(recv_mgstrt* _recv_mgstrt)
{
	_ControlBoard[_recv_mgstrt->srcaddr + 4].Motor[_recv_mgstrt->motoraddr].Send_OK[_recv_mgstrt->cmdnum] = true;
}


/*
 * 分控板接收处理，地址：1
 * 参数：接收数据结构体
 */
void ControlBoardRecv(recv_mgstrt* _recv_mgstrt)
{
	switch(_recv_mgstrt->cmdnum){
	case 0x01://读版本
		ReadVersion_Recv(_recv_mgstrt);
		break;
	case OpenOrCloseScan_Command://打开/关闭扫码枪
		OpenOrCloseScan_Recv(_recv_mgstrt);
		break;
	case Scan_Recv_Command://扫码返回
		Scan_Recv(_recv_mgstrt);
		break;
	case OpenOrCloseScan_2_Command://打开/关闭扫码枪
		OpenOrCloseScan_2_Recv(_recv_mgstrt);
		break;
	case Scan_Recv_2_Command://扫码返回
		Scan_Recv_2(_recv_mgstrt);
		break;
	case OpenBuzzer_Command:
		BuzzerOpen_Recv(_recv_mgstrt);
		break;
	}
}


/*
 * 接收下位机数据
 */

void *myrecv_can()
{
	char buff[4096];
	memset(buff, 0, sizeof(buff));
	//char recv_queue_buf_tmp[40];
	int m = 0;
	while(1)
	{
		m = deQueue(PQueue_RecvCAN, buff);
		if(m == 2)//出队成功
		{
			ControlBoardRecvAnalysis(buff, ControlBoardRecv);
		}

		usleep(1);
	}
}

/*
 * 成功返回1，失败返回 分控板地址<<16 && 电机地址<<8 && 命令号
 */
int CanSendCommand(int dstaadr, int motoraddr, int cmdnum, unsigned int *Datanum)
{
	Data _Data = MergeData(Datanum);
	uint32_t tmp_ID = 0;
	tmp_ID = (dstaadr << 20) | (motoraddr << 16) |  cmdnum ;
	struct timeval Time_CAN_Send;
	struct timeval Time_CAN_Now;
	_ControlBoard[dstaadr + 4].Motor[motoraddr].Send_OK[cmdnum] = false;//此处＋4是为了存储，前四个给串口用了，dstaadr = 1
	int index = 0;
	for(index = 0; index < 3; index++)
	{
		//printf("index = %d\n",index);
		gettimeofday(&Time_CAN_Send, NULL);//获取发送时间
		//pthread_mutex_lock(&CanSendmut1);
		Can_Write(fd_can_800K, 1, tmp_ID, 0, 0x8, _Data.Data1, _Data.Data2);
		if(tmp_ID == 0x0015001C)
		{
			printf("\r\n");
		}
		//my_mutex_lock(Can_fd, 0);
		//SaveLogToQueue(PQueue_SaveLog, &PQueue_SaveLog_mut, "%s : send to mortor : %08x,00000008,%08x,%08x\r\n", buf, tmp_ID, _Data.Data1, _Data.Data2);
		while(1)
		{
			gettimeofday(&Time_CAN_Now, NULL);//获取发送时间
			if(_ControlBoard[dstaadr + 4].Motor[motoraddr].Send_OK[cmdnum] == true)

			{
				_ControlBoard[dstaadr + 4].Motor[motoraddr].Send_OK[cmdnum] = false;
				//pthread_mutex_unlock(&CanSendmut);
				return 1;
			}
			if((1000000 * (Time_CAN_Now.tv_sec - Time_CAN_Send.tv_sec) + Time_CAN_Now.tv_usec - Time_CAN_Send.tv_usec) > 500000)
			{
				printf("Send Again : %d dstaadr:%d,motoraddr:%d,cmdnum%d\n",
						index, dstaadr, motoraddr,cmdnum);
				break;
			}
			usleep(1);
		}
	}
	return (dstaadr << 16) | (motoraddr << 8) | (cmdnum);
}


void Can_open()
{
	PQueue_RecvCAN = (queue *) malloc(sizeof(queue));//队列申请内存
	initQueue(PQueue_RecvCAN);


	fd_can_800K = Can_Open(can_1, CAN_BIT_800K, can_func_call, 0);
}

