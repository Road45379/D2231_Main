/*
 * Track_Mode.c
 *
 * 轨道模块 05
 *
 *履带模块
 *  Created on: 2025-12-23
 *      Author: ThinkPad
 */
#include "Track_Mode.h"
#include "TurnPlate_2_OP.h"
#include "TurnPlate_3_OP.h"
#define MOTOR_STARTORSTOP   		0x11   	//上位机指令-轨道电机启停
#define MOTOR_READ_MOTOR_EXIST   	0x0B   	//上位机指令-查询轨道电机
#define MOTOR_SET_MOTOR_EXIST   	0x12   	//上位机指令-发送轨道电机地址
#define READ_WRITE_MOTOR_PARA	    0x08	//读写电机参数
#define READ_MOTOR_STATE	   		0x13	//读电机状态


void TarskControlModule_MotorStartOrStop(NetCmd *cmd)
{
	int start_or_stop = 0;
	start_or_stop = AsciiToHex(cmd->pvar[0]);//0 停止 ,1 启动
	static int i = 0;
	//static int n = 0;
	if(_State_Moudle.State_TarskControlModule1 == State_NoBusy)
	{
		_State_Moudle.State_TarskControlModule1 = State_Busy;
		if(BIT(Track_Motor_Exist[i / 32], i % 32) > 0)//i = 0, 1号电机存在
		{
			if(0 != UartSend(fd_RS485_index_3, Tarck_Mode_Motor_Add_1, MOTOR_COM_MOMENT_MODE, 1, &cmd->pvar[0]))
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TarskControlCommandAdd, PackByte(&cmd->pvar[0])));
				_State_Moudle.State_TarskControlModule1 = State_NoBusy;
				i = 0;

			}
		}
		i++;
	}else
	{
		if(i < 0x80)
		{
			if(BIT(Track_Motor_Exist[i / 32], i % 32) > 0)
			{
				if(0 != UartSend(fd_RS485_index_3, Tarck_Mode_Motor_Add_1 + i, MOTOR_COM_MOMENT_MODE, 1, &cmd->pvar[0]))
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TarskControlCommandAdd, PackByte(&cmd->pvar[0])));
					_State_Moudle.State_TarskControlModule1 = State_NoBusy;
					i = 0;
					return;
				}
			}
			i++;
		}else
		{
			Set_Trak_State(start_or_stop);
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			_State_Moudle.State_TarskControlModule1 = State_NoBusy;
			i = 0;
		}
	}
}

/*
 * 搜索轨道电机是否存在，从0x01扫到0xF0，（不含0x3F和0x4F）
 */
void Read_Track_Motor_Exist(NetCmd *cmd)
{
	int i = 1;
	char version[4][9] = {""};
	for(; i < 0x80; i++)
	{
		if(i == RFID_4_ADDR || i == RFID_3_ADDR)
		{
			BIT_Clear(Track_Motor_Exist[(i - 1) / 32], (i - 1) % 32);
		}else{
			if(UartSend(fd_RS485_index_3, i, MOTOR_COM_READ_VERSION, 0) != 0)
			{
				//通讯失败，不存在
				BIT_Clear(Track_Motor_Exist[(i - 1) / 32], (i - 1) % 32);
			}else
			{
				BIT_Set(Track_Motor_Exist[(i - 1) / 32], (i - 1) % 32);
			}
		}
	}
	for(i = 0; i < 4; i++)
	{
		sprintf(version[i], "%08X", Track_Motor_Exist[i]);
	}
	Eth_Send_Queue(cmd, 0, 0xFF, 5, 0x0000, version[3], version[2], version[1], version[0]);
}

//读写电机参数
void TarskControlModule_MotorPara(NetCmd *cmd)
{
	if(_State_Moudle.State_TarskControlModule1 == State_NoBusy)
	{
		_State_Moudle.State_TarskControlModule1 = State_Busy;
		if(AsciiToHex(cmd->pvar[0]) == 0)//读
		{
			if(UartSend(fd_RS485_index_3, PackByte(&cmd->pvar[1]), MOTOR_COM_READ_PARA, 0) != 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TarskControlCommandAdd, PackByte(&cmd->pvar[0])));
			}else
			{
				char buf[128] = {0};

				int len = PackByte(_ControlBoard[fd_RS485_index_3].Motor[PackByte(&cmd->pvar[1])].point);
				if(len != 126)
				{

				}else
				{
					memcpy(buf, _ControlBoard[fd_RS485_index_3].Motor[PackByte(&cmd->pvar[1])].point + 2, 48);
					memcpy(buf + 48, _ControlBoard[fd_RS485_index_3].Motor[PackByte(&cmd->pvar[1])].point + 58, len - 56);
				}

				Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, buf);
			}
		}else if(AsciiToHex(cmd->pvar[0]) == 1)//写
		{
			char buf[128] = {0};
			int len = 118;
			memcpy(buf, &cmd->pvar[3], len);
			if(UartSend(fd_RS485_index_3, PackByte(&cmd->pvar[1]), MOTOR_COM_WRITE_PARA, 1, buf) != 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TarskControlCommandAdd, PackByte(&cmd->pvar[0])));
			}else
			{
				//保存参数
				if(UartSend(fd_RS485_index_3, PackByte(&cmd->pvar[1]), MOTOR_COM_SAVE_PARA, 0) != 0)
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TarskControlCommandAdd, PackByte(&cmd->pvar[0])));
				}else
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, 0x0000);
				}
			}

		}else
		{

		}
		_State_Moudle.State_TarskControlModule1 = State_NoBusy;
	}
}

//读电机状态
void TarskControlModule_ReadState(NetCmd *cmd)
{
	if(_State_Moudle.State_TarskControlModule1 == State_NoBusy)
	{
		_State_Moudle.State_TarskControlModule1 = State_Busy;
		if(UartSend(fd_RS485_index_3, PackByte(&cmd->pvar[0]), MOTOR_COM_READ_STATE, 0) != 0)
		{
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TarskControlCommandAdd, PackByte(&cmd->pvar[0])));
		}else
		{
			char buf [9] = {0};
			sprintf(buf, "%02X", PackByte(_ControlBoard[fd_RS485_index_3].Motor[PackByte(&cmd->pvar[0])].point));
			Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, buf);
		}
		_State_Moudle.State_TarskControlModule1 = State_NoBusy;
	}
}

void TarskControlModule(NetCmd *cmd)
{
	switch(cmd->code){
	case READ_VERSION:
		if(UartSend(fd_RS485_index_3, PackByte(&cmd->pvar[0]), MOTOR_COM_READ_VERSION, 0) != 0)
		{
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TarskControlCommandAdd, PackByte(&cmd->pvar[0])));
		}else
		{
			char version[4][9] = {""};
			uint8_t len = PackByte(_ControlBoard[fd_RS485_index_3].Motor[PackByte(&cmd->pvar[0])].point);
			len += 2;
			int i = 0;
			int n = (len / 8) + (len % 8 == 0 ? 0 : 1);
			for(i = 0; i < n; i++)
			{
				if(i == n - 1)
				{
					if((len % 8) == 0)
					{
						memcpy(version[i],  _ControlBoard[fd_RS485_index_3].Motor[PackByte(&cmd->pvar[0])].point + (8 * i), 8);
					}else
					{
						memcpy(version[i],  _ControlBoard[fd_RS485_index_3].Motor[PackByte(&cmd->pvar[0])].point + (8 * i), len % 8);
						memset(version[i] + (len % 8), '0', 8 - (len % 8));
					}

				}else
				{
					memcpy(version[i],  _ControlBoard[fd_RS485_index_3].Motor[PackByte(&cmd->pvar[0])].point + (8 * i), 8);
				}
			}

			Eth_Send_Queue(cmd, 0, 0xFF, n + 1, 0x0000, version[0], version[1], version[2], version[3]);
		}
		break;
	case MOTOR_STARTORSTOP://轨道电机启停
		Set_Trak_State(AsciiToHex(cmd->pvar[0]));
		TarskControlModule_MotorStartOrStop(cmd);
		break;
	case MOTOR_READ_MOTOR_EXIST://查询轨道电机是否存在
		Read_Track_Motor_Exist(cmd);
		break;
	case MOTOR_SET_MOTOR_EXIST://上位机发送哪些轨道电机存在
	{
		int i = 0;
		for(i = 0; i < 4; i++)
		{
			Track_Motor_Exist[i] = Pack8Byte(&cmd->pvar[24 - (i * 8)]);
		}
		Eth_Send_Queue(cmd, 0, 0xFF, 1, 0x0000);
		break;
	}
	case READ_WRITE_MOTOR_PARA://读写电机参数
		TarskControlModule_MotorPara(cmd);
		break;
	case READ_MOTOR_STATE://读电机状态
		TarskControlModule_ReadState(cmd);
		break;

	}
}
