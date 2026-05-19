/*
 * TurnPlate_2.c
 *
 *  Created on: 2025-12-30
 *      Author: ThinkPad
 */
#include "TurnPlate_2.h"
#include "TurnPlate_2_OP.h"

#define MOTOR_RESET				   0x03//电机复位
#define MOTOR_FREE				   0x04//电机释放
#define SET_TurnPlate_1_PLACE      0x11//设置转盘位置
#define MOVE_TEST_TUBE    	   	   0x1B//移动试管
#define SAVE_POINT				   0x07//读写定位参数
#define READ_WRITE_MOTOR_PARA	   0x08//读写电机参数


void TurnPlate_2_Module_ResetMotor(NetCmd *cmd)//转盘1_复位电机
{
	if(_State_Moudle.State_TurnPlate_2_Module == State_NoBusy )
	{
		_State_Moudle.State_TurnPlate_2_Module = State_Busy;

		//转盘复位
		UartSend(fd_RS485_index_3, TurnPlate_2_Mode_Motor_Add, MOTOR_COM_RESET, 0);
		TurnPlate_2_Module_Num = STEP_1;
	}else
	{
		if(TurnPlate_2_Module_Num == STEP_1)
		{
			//等转盘复位完成
			UartSend(fd_RS485_index_3, TurnPlate_2_Mode_Motor_Add, MOTOR_COM_READRESETSTATE, 0);
			if(PackByte(_ControlBoard[fd_RS485_index_3].Motor[TurnPlate_2_Mode_Motor_Add].point) == 0x01)//转盘复位完成
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
				_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
			}
		}
	}
}


/*
 * 转盘移动
 * _steps 坐标
 *
 * return :
 * 1 ： 通讯故障
 * 2 ： 撞击
 * 0 ：正常结束
 * 5 ： 运行中
 *
 */
int TurnPlate_2_Move(char * _steps)
{
	char err;
	if(_State_Moudle.State_TurnPlate_2_Module == State_NoBusy)
	{
		_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		char _steps_buf[9] = {0};
		memcpy(_steps_buf, _steps, 8);
		if(UartSend(fd_RS485_index_3, TurnPlate_2_Mode_Motor_Add, MOTOR_COM_SETSTEPS,1, _steps_buf) != 0)
		{
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
			return 1;
		}
		TurnPlate_2_Module_Num = STEP_1;
		return 5;
	}else{
		if(TurnPlate_2_Module_Num == STEP_1)//等转盘移动完成
		{
			if(UartSend(fd_RS485_index_3, TurnPlate_2_Mode_Motor_Add, MOTOR_COM_READ_IN_PLACE, 0) != 0)
			{
				_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
				return 1;
			}
			if((err = PackByte(_ControlBoard[fd_RS485_index_3].Motor[TurnPlate_2_Mode_Motor_Add].point)) != 0x00)//0为移动中
			{
				//2撞击
				_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
				return err == 2 ? 2 : 0;
			}else
			{
				return 5;
			}
		}
	}
}

/*
 * 读转盘位置
 *
 * return :
 * 1 : 通讯故障
 * 0 ：正常结束
 *5:组件忙
 */
int TurnPlate_2_Read_Place()
{
	if(_State_Moudle.State_TurnPlate_2_Module == State_NoBusy)
	{
		_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		if(UartSend(fd_RS485_index_3, TurnPlate_2_Mode_Motor_Add, MOTOR_COM_READSTEPS, 0) != 0)
		{
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
			return 1;
		}else
		{
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
			return 0;
		}
	}else
	{
		return 5;
	}
}

void TurnPlate_2_Module_LockMotor(NetCmd *cmd)//转盘1_锁定/释放电机
{
	if(_State_Moudle.State_TurnPlate_2_Module == State_NoBusy)
	{
		_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		char Lock[2] = {0};
		Lock[0] = cmd->pvar[0];
		if(UartSend(fd_RS485_index_3, TurnPlate_2_Mode_Motor_Add, MOTOR_COM_FREE, 1, &Lock) != 0)
		{
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
			//报错
		}else
		{
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
		}
	}
}

void TurnPlate_2_Module_Set_Place(NetCmd *cmd)
{
	if(AsciiToHex(cmd->pvar[0]) == 0x01)//设置位置
	{
		int ret = TurnPlate_2_Move(&cmd->pvar[1]);
		switch(ret)
		{
		case 0:
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			break;
		case 1:
			break;
		case 5:
			break;
		}
	}else if(AsciiToHex(cmd->pvar[0]) == 0x00)//读位置
	{
		int ret = TurnPlate_2_Read_Place();
		if(ret == 1)
		{
			//通讯失败
		}else
		{
			char _Point[16] = "";
			strncat(_Point, _ControlBoard[fd_RS485_index_3].Motor[TurnPlate_2_Mode_Motor_Add].point, 8);
			Eth_Send_Queue(cmd, 0, 0xFF, 2, 0000, _Point);
		}
	}
}

/*
 * 移动试管
 */
void TurnPlate_2_Module_MoveTestTube(NetCmd *cmd)
{
	unsigned int Src_hole0x04 = PackByte((char*) &cmd->pvar[0]);
	unsigned int Des_hole0x04 = PackByte((char*) &cmd->pvar[2]);
	_Moudle_Point.TurnPlate_2_Module_point = turntable2Pos->portPos[Src_hole0x04 + 1];
	char _point_buf[9] = {0};
	sprintf(_point_buf, "%08X", _Moudle_Point.TurnPlate_2_Module_point);
	static int a = 0;
	static struct timeval Time_start;
	static struct timeval Time_now;
	int err = 0;
	if(a == 0)
	{
		//运动到起始位置
		_Moudle_Point.TurnPlate_2_Module_point = turntable2Pos->portPos[Src_hole0x04 - 1];
		sprintf(_point_buf, "%08X", _Moudle_Point.TurnPlate_2_Module_point);
		err = TurnPlate_2_Move(_point_buf);
		switch(err)
		{
		case 0:
			a = 1;
			gettimeofday(&Time_start, NULL);
			break;
		case 1:
			break;
		case 5:
			break;
		}
	}
	if(a == 1)
	{
		//延时
		_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, turntable2Pos->waitingTime[Src_hole0x04 / 2]) == 0)
		{
			a = 2;//超时完成
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
		}
	}
	if(a == 2)
	{
		//移动到目标位置
		_Moudle_Point.TurnPlate_2_Module_point = turntable2Pos->portPos[Des_hole0x04 - 1];
		sprintf(_point_buf, "%08X", _Moudle_Point.TurnPlate_2_Module_point);
		err = TurnPlate_2_Move(_point_buf);
		switch(err)
		{
		case 0:
			a = 3;
			gettimeofday(&Time_start, NULL);
			break;
		case 1:
			break;
		case 5:
			break;
		}
	}
	if(a == 3)
	{
		_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, turntable2Pos->waitingTime[Des_hole0x04 / 2]) == 0)
		{
			a = 0;//超时完成
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
		}
	}
}

//读写电机参数
void TurnPlate_2_Module_MotorPara(NetCmd *cmd)
{
	if(_State_Moudle.State_TurnPlate_2_Module == State_NoBusy)
	{
		_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		if(AsciiToHex(cmd->pvar[0]) == 0)//读
		{
			if(UartSend(fd_RS485_index_3, PackByte(&cmd->pvar[1]), MOTOR_COM_READ_PARA, 0) != 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_2_ModuleCommandAdd, ManipulatorModule_Motor_Err[PackByte(&cmd->pvar[1]) - 1].Communication_Err));
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
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_2_ModuleCommandAdd, ManipulatorModule_Motor_Err[PackByte(&cmd->pvar[1]) - 1].Communication_Err));
			}else
			{
				//保存参数
				if(UartSend(fd_RS485_index_3, PackByte(&cmd->pvar[1]), MOTOR_COM_SAVE_PARA, 0) != 0)
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_2_ModuleCommandAdd, ManipulatorModule_Motor_Err[PackByte(&cmd->pvar[1]) - 1].Communication_Err));
				}else
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, 0x0000);
				}
			}
		}else
		{

		}
		_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
	}
}

void TurnPlate_2_Module(NetCmd *cmd)
{
	switch(cmd->code){
	case READ_VERSION:
		if(PackByte(&cmd->pvar[0]) == 0x01)//2号转盘（主控中间转盘）
		{
			if(UartSend(fd_RS485_index_3, TurnPlate_2_Mode_Motor_Add, MOTOR_COM_READ_VERSION, 0) != 0)
			{
				//通讯失败，报错
			}else
			{
				char version[4][9] = {""};
				uint8_t len = PackByte(_ControlBoard[fd_RS485_index_3].Motor[TurnPlate_2_Mode_Motor_Add].point);
				len += 2;
				int i = 0;
				int n = (len / 8) + (len % 8 == 0 ? 0 : 1);
				for(i = 0; i < n; i++)
				{
					if(i == n - 1)
					{
						if((len % 8) == 0)
						{
							memcpy(version[i],  _ControlBoard[fd_RS485_index_3].Motor[TurnPlate_2_Mode_Motor_Add].point + (8 * i), 8);
						}else
						{
							memcpy(version[i],  _ControlBoard[fd_RS485_index_3].Motor[TurnPlate_2_Mode_Motor_Add].point + (8 * i), len % 8);
							memset(version[i] + (len % 8), '0', 8 - (len % 8));
						}

					}else
					{
						memcpy(version[i],  _ControlBoard[fd_RS485_index_3].Motor[TurnPlate_2_Mode_Motor_Add].point + (8 * i), 8);
					}
				}

				Eth_Send_Queue(cmd, 0, 0xFF, n + 1, 0x0000, version[0], version[1], version[2], version[3]);
			}
		}else if(PackByte(&cmd->pvar[0]) == 0x02)//2号转盘下的IC卡
		{
			if(UartSend(fd_RS485_index_3, RFID_3_ADDR, MOTOR_COM_READ_VERSION, 0) != 0)
			{
				//有通讯故障，报错
			}else
			{
				char version[4][9] = {""};
				uint8_t len = PackByte(_ControlBoard[fd_RS485_index_3].Motor[RFID_3_ADDR].point);
				len += 2;
				int i = 0;
				int n = (len / 8) + (len % 8 == 0 ? 0 : 1);
				for(i = 0; i < n; i++)
				{
					if(i == n - 1)
					{
						if((len % 8) == 0)
						{
							memcpy(version[i],  _ControlBoard[fd_RS485_index_3].Motor[RFID_3_ADDR].point + (8 * i), 8);
						}else
						{
							memcpy(version[i],  _ControlBoard[fd_RS485_index_3].Motor[RFID_3_ADDR].point + (8 * i), len % 8);
							memset(version[i] + (len % 8), '0', 8 - (len % 8));
						}

					}else
					{
						memcpy(version[i],  _ControlBoard[fd_RS485_index_3].Motor[RFID_3_ADDR].point + (8 * i), 8);
					}
				}

				Eth_Send_Queue(cmd, 0, 0xFF, n + 1, 0x0000, version[0], version[1], version[2], version[3]);
			}
		}
		break;
	case MOTOR_RESET://复位
		TurnPlate_2_Module_ResetMotor(cmd);
		break;
	case SET_TurnPlate_1_PLACE://设置转盘位置
		TurnPlate_2_Module_Set_Place(cmd);
		break;
	case MOTOR_FREE://机械手_锁定/释放电机
		TurnPlate_2_Module_LockMotor(cmd);
		break;
	case SAVE_POINT://读写定位参数
		if(AsciiToHex(cmd->pvar[0]) == 1)//写参数
		{
			//定位参数
			unsigned int pvarlength = PackWord((char*) &cmd->pvar[2]) * 2;
			char point[4096] = "0";
			int n = 0;
			int i = 0;
			for(i = 0; i < pvarlength;)
			{
				point[n] = PackByte((char*) &cmd->pvar[6 + i]);
				i = i+2;
				n++;
			}
			memcpy(turntable2Pos, point, sizeof(TransportTurntablePos));
			SavePointConfigure(Filename_config, cmd);
			char send_buf0x07[16] = "";
			strncat(send_buf0x07, (char*)&cmd->pvar[2], 4);
			Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, send_buf0x07);
		}else
		{
			//读参数
			char s[1024] = "";
			ReadPointConfigure("POINT", "06", Filename_config, s);
			Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, s);
		}
		break;
	case MOVE_TEST_TUBE://移动试管
		TurnPlate_2_Module_MoveTestTube(cmd);
		break;
	case READ_WRITE_MOTOR_PARA://读写电机参数
		TurnPlate_2_Module_MotorPara(cmd);
		break;
	}
}
