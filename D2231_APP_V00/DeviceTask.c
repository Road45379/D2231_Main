/*
 * DeviceTask.c
 *
 *仪器所有运行流程
 *
 *  Created on: 2025-12-23
 *      Author: ThinkPad
 */
#include "Track_Mode.h"
#include "ManipulatorModule.h"
#include "TurnPlate_1.h"
#include "TurnPlate_2.h"
#include "TurnPlate_3.h"
#include "SampleShelf.h"
#include "GPIO_Driver.h"
#include "RFID.h"
#include "TurnPlate_2_OP.h"
#include "TurnPlate_3_OP.h"
#include "ControlBoard.h"

#define _Timeout   50//ms

char Sensor_state[6] = {0};//传感器状态
char Sensor_state_change[6] = {0};//传感器状态变化计数

int Sensor_1_inhand = 0;//传感器1处理中

void ReadIO(NetCmd *_NetCmd)
{
	int tmp = IOInput() & 0x3F;
	static int tmp_IO = 0;
	static int i = 0;
	static struct timeval Time_start;
	static struct timeval Time_now;
	int n = 0;
	int m = 0;
	int err = 0;
	if(BIT(tmp, 0) == 0)
	{
		if(Get_CarID_Can_Return() == 0)
		{
			if(Sensor_1_inhand == 0)//表示传感器1当前未在处理
			{
					Sensor_1_inhand = 1;
					gettimeofday(&Time_start, NULL);
			}else if(Sensor_1_inhand == 1)//表示传感器1当前在处理中
			{
				gettimeofday(&Time_now, NULL);
				if(My_timeout(&Time_start, &Time_now, _Timeout) == 0)
				{
					int Sensor_2_state = 0;
					if(BIT(tmp, 1) == 0)//光耦2触发
					{
						Sensor_2_state = 1;
					}
					err = Read_RFID_Barcode(RFID_1_ADDR);
					if(err == 0)//读
					{
						Sensor_1_inhand = 0;
						char buf[1024] = {0};
						sprintf(buf, "%08X%02X",  _message_TurnPlate_1_1.CAR_ID,_message_TurnPlate_1_1.BarCode_len);
						memcpy(buf + 10, _message_TurnPlate_1_1.BarCode, _message_TurnPlate_1_1.BarCode_len);
						memcpy(buf + 10 + _message_TurnPlate_1_1.BarCode_len, _message_TurnPlate_1_1.tarck, 89);//轨迹信息
						sprintf(buf + 99 + _message_TurnPlate_1_1.BarCode_len, "%01X",  Sensor_2_state);//是否有试管
						int i =  _message_TurnPlate_1_1.message_len - 2 - _message_TurnPlate_1_1.BarCode_len;
						//+样本状态2，+92是否测试异常6
						if(i > 0 && i < 200)
						{
							memcpy(buf + 100 + _message_TurnPlate_1_1.BarCode_len, _message_TurnPlate_1_1.message, i);
						}
						Eth_Send_Queue(_NetCmd, 0, 0xFF, 2, 0000, buf);
						Set_CarID_Can_Return(1);
					}else if(err  == 3)//通讯失败
					{
						//报错
						Eth_Send_Queue(_NetCmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Unload_RFID_Communication_Err));
					}else if(err  == 1)//寻卡失败
					{
						//报错
						Eth_Send_Queue(_NetCmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Unload_RFID_FindCard_Err));
					}
				}
			}
		}
	}
	Turntable_2_Sensor_3(tmp);
	Turntable_2_Sensor_4(tmp);
	Turntable_3_Sensor_5(tmp);
	Turntable_3_Sensor_6(tmp);

/*

	if(tmp_IO != tmp)//有变化
	{
		m = calc_diff_bit(tmp_IO, tmp);
		for(n = 0; n < 6; n++)
		{
			if(BIT(m, n) != 0)
			{
				if(BIT(tmp, n) == 0)//当前是0，表示从1变为0， 触发一次（只记下降沿）
				{
					Sensor_state_change[n]++;
				}
			}
		}

	}
	tmp_IO = tmp;

	if(Sensor_1_inhand == 0)//表示传感器1当前未在处理
	{
		if(Sensor_state_change[0] > 0)//表示传感器1触发过
		{
			Sensor_1_inhand = 1;
			Sensor_state_change[0]--;
			gettimeofday(&Time_start, NULL);
		}
	}else if(Sensor_1_inhand == 1)//表示传感器1当前在处理中
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, _Timeout) == 0)
		{
			int Sensor_2_state = 0;
			if(BIT(tmp, 1) == 0)//光耦2触发
			{
				Sensor_2_state = 1;
			}
			if(Read_RFID_Barcode(RFID_1_ADDR) == 0)//读
			{
				Sensor_1_inhand = 0;
				char buf[1024] = {0};
				sprintf(buf, "%01X%08X%02X", Sensor_2_state, _message_TurnPlate_1_1.CAR_ID,_message_TurnPlate_1_1.BarCode_len);
				memcpy(buf + 11, _message_TurnPlate_1_1.BarCode, _message_TurnPlate_1_1.BarCode_len);

				Eth_Send_Queue(_NetCmd, 0, 0xFF, 2, 0000, buf);
			}
		}
	}
*/

	/*if(tmp_s < 0)
	{
		FinishRecv(cmd_AutoSend_IO, 1, 0xFF, 1, State_Manipulator_CommunicationError);
	}
	if(tmp_IO != tmp || tmp_start != tmp_s)
	{
		char buf[9] = "";
		sprintf(buf, "%03X%01X0000", tmp, tmp_s);
		FinishRecv(cmd_AutoSend_IO, 0, 0xFF, 2, 0x0000, buf);
	}*/
}

/*
 * 把字符串解析成结构体
 */
void ParsingStringToNetCmd(char *buf, NetCmd *_NetCmd)
{
	//NetCmd *_NetCmd = NULL;
	char Command_viraddr[3] = "";
	char Command_code[3] = "";
	char Command_pvar[4096] = "";
	char NetCmdHead[16] = "";
	//_NetCmd = (NetCmd *) malloc(sizeof(NetCmd));
	memset(_NetCmd, 0, sizeof(NetCmd));
	//printf("Recv from PC = %s", buf);
	memset(NetCmdHead, 0, sizeof(NetCmdHead));
	memcpy(NetCmdHead, buf, 13);
	memset(Command_viraddr, 0, sizeof(Command_viraddr));
	memcpy(Command_viraddr, buf + 9, 2);
	memset(Command_code, 0, sizeof(Command_code));
	memcpy(Command_code, buf + 11, 2);
	memset(Command_pvar, 0, sizeof(Command_pvar));
	memcpy(Command_pvar, buf + 13, strlen(buf + 13) - 6);
	_NetCmd->viraddr = 0;
	_NetCmd->viraddr = htoi(Command_viraddr);
	_NetCmd->code = 0;
	_NetCmd->code = htoi(Command_code);
	memset(_NetCmd->pvar, 0, NET_CMD_LENTH_MSG);
	strcpy(_NetCmd->pvar, Command_pvar);
	_NetCmd->cnt = strlen(_NetCmd->pvar);
	memset(_NetCmd->netcmdhead, 0, sizeof(_NetCmd->netcmdhead));
	strcpy(_NetCmd->netcmdhead, NetCmdHead);
}
char Module_queue_buf[1024] = "";
void My_deQueue(volatile queue *PQueue, int val)
{
	char buf[64] = "";
	switch(val){
	case 0:
		if(_State_Moudle.State_TarskControlModule1 == State_NoBusy)
		{

			memset(Module_queue_buf, 0, sizeof(Module_queue_buf));
			deQueue(PQueue, Module_queue_buf);
		}
		break;
	case 1:
		if(_State_Moudle.State_Manipulator == State_NoBusy)
		{
			ManipulatorModule_Num = STEP_0;
			memset(Module_queue_buf, 0, sizeof(Module_queue_buf));
			deQueue(PQueue, Module_queue_buf);
		}
		break;
	case 2:
		if(_State_Moudle.State_TurnPlate_1_Module == State_NoBusy)
		{
			TurnPlate_1_Module_Num = STEP_0;
			memset(Module_queue_buf, 0, sizeof(Module_queue_buf));
			deQueue(PQueue, Module_queue_buf);
		}
		break;
	case 3:
		if(_State_Moudle.State_TurnPlate_2_Module == State_NoBusy)
		{
			TurnPlate_2_Module_Num = STEP_0;
			memset(Module_queue_buf, 0, sizeof(Module_queue_buf));
			deQueue(PQueue, Module_queue_buf);
		}
		break;
	case 4:
		if(_State_Moudle.State_TurnPlate_3_Module == State_NoBusy)
		{
			TurnPlate_3_Module_Num = STEP_0;
			memset(Module_queue_buf, 0, sizeof(Module_queue_buf));
			deQueue(PQueue, Module_queue_buf);
		}
		break;
	case 5:
		if(_State_Moudle.State_SampleShelf_Module == State_NoBusy)
		{
			SampleShelf_Module_Num = STEP_0;
			memset(Module_queue_buf, 0, sizeof(Module_queue_buf));
			deQueue(PQueue, Module_queue_buf);
		}
		break;
	case 6:
		if(_State_Moudle.State_ControlBoard_Module[0] == State_NoBusy)
		{
			ControlBoard_Motor_Num[0] = STEP_0;
			memset(Module_queue_buf, 0, sizeof(Module_queue_buf));
			deQueue(PQueue, Module_queue_buf);
		}
		break;
	case 7:
		if(_State_Moudle.State_ControlBoard_Module[1] == State_NoBusy)
		{
			ControlBoard_Motor_Num[1] = STEP_0;
			memset(Module_queue_buf, 0, sizeof(Module_queue_buf));
			deQueue(PQueue, Module_queue_buf);
		}
		break;
	}
}


static NetCmd *ParsingStringToNetCmd_NetCmd = NULL;//解析完成的指令
/*
 * 命令处理
 */
void CommandDispose(volatile queue *PQueue, void (*ModuleFunc)(NetCmd*), int val)
{
	if(isEmpityQueue(PQueue) == 3)
	{
		ParsingStringToNetCmd((char*)PQueue->pBase[PQueue->front] + 4, ParsingStringToNetCmd_NetCmd);
		ModuleFunc(ParsingStringToNetCmd_NetCmd);
		My_deQueue(PQueue, val);
	}
}


void DeviceTask_Thread()
{
	ParsingStringToNetCmd_NetCmd = (NetCmd *) malloc(sizeof(NetCmd));//解析完成的指令
	while(1)
	{
		//此处添加复位动作
		//复位流程：机械手复位->Z轴复位->X轴、Y轴、三个转盘复位
		if(reset == 0)//开始复位
		{
			my_memset((int*)&_State_Moudle.State_TarskControlModule1, State_NoBusy, sizeof(State_Moudle)/sizeof(int));
			//电爪复位
			UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, MOTOR_COM_RESET, 0);
			reset = 1;
		}else if(reset == 1)
		{
			//查电爪复位状态，等待复位完成
			UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, MOTOR_COM_READRESETSTATE, 0);
			if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point) == 0x01)//电爪复位完成，复位Z轴
			{
				UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_RESET, 0);
				reset = 2;
			}
		}else if(reset == 2)
		{
			//查询Z轴复位状态
			UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_READRESETSTATE, 0);
			if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_z].point) == 0x01)//Z轴复位完成，复位其他
			{
				UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_x, MOTOR_COM_RESET, 0);
				UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_y, MOTOR_COM_RESET, 0);
				UartSend(fd_RS485_index_1, TurnPlate_1_Mode_Motor_Add, MOTOR_COM_RESET, 0);
				UartSend(fd_RS485_index_3, TurnPlate_2_Mode_Motor_Add, MOTOR_COM_RESET, 0);
				UartSend(fd_RS485_index_3, TurnPlate_3_Mode_Motor_Add, MOTOR_COM_RESET, 0);
				reset = 3;
			}
		}else if(reset == 3)
		{
			//查询其他轴复位状态
			UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_x, MOTOR_COM_READRESETSTATE, 0);
			UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_y, MOTOR_COM_READRESETSTATE, 0);
			UartSend(fd_RS485_index_1, TurnPlate_1_Mode_Motor_Add, MOTOR_COM_READRESETSTATE, 0);
			UartSend(fd_RS485_index_3, TurnPlate_2_Mode_Motor_Add, MOTOR_COM_READRESETSTATE, 0);
			UartSend(fd_RS485_index_3, TurnPlate_3_Mode_Motor_Add, MOTOR_COM_READRESETSTATE, 0);
			if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_x].point) == 0x01
					&& PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_y].point) == 0x01
					&& PackByte(_ControlBoard[fd_RS485_index_1].Motor[TurnPlate_1_Mode_Motor_Add].point) == 0x01
					&& PackByte(_ControlBoard[fd_RS485_index_3].Motor[TurnPlate_2_Mode_Motor_Add].point) == 0x01
					&& PackByte(_ControlBoard[fd_RS485_index_3].Motor[TurnPlate_3_Mode_Motor_Add].point) == 0x01)//其他轴复位完成，复位完成
			{
				Eth_Send_Queue(_NetCmd_reset, 0, 0xFF, 1, 0000);
				_State_Moudle.State_DeviceReset = 1;//整机状态-复位完成
				free(_NetCmd_reset);
				_NetCmd_reset = NULL;
				reset = 6;
				//初始化测试信息缓冲区和异步指令缓冲区
				ClearList(&list);
				ClearAsyncCmdList(&asyncCmdlist);
			}
		}else if(reset == 6)//
		{
			ReadIO(cmd_AutoSend_Sensor_State);
			SampleShelf_ReadState(cmd_AutoSend_SampleShelf_State);//读电磁铁板状态
			Get_ControlBoard_State();//分控心跳包
			CommandDispose(PQueue_TarskControlModule1,  TarskControlModule, 0);
			CommandDispose(PQueue_ManipulatorModule1,  ManipulatorModule, 1);
			CommandDispose(PQueue_TurnPlate_1_Module,  TurnPlate_1_Module, 2);
			CommandDispose(PQueue_TurnPlate_2_Module,  TurnPlate_2_Module, 3);
			CommandDispose(PQueue_TurnPlate_3_Module,  TurnPlate_3_Module, 4);
			CommandDispose(PQueue_SampleShelf_Module,  SampleShelfModule, 5);
			CommandDispose(PQueue_ControlBoard_1_Module,  ControlBoard_1_Module, 6);
			CommandDispose(PQueue_ControlBoard_2_Module,  ControlBoard_1_Module, 7);
		}
		usleep(20000);
	}
}


