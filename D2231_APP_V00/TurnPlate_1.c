/*
 * TurnPlate_1.c
 *
 *转盘1（靠近机械手的转盘）
 *  Created on: 2025-12-29
 *      Author: ThinkPad
 */

#include "TurnPlate_1.h"
#include "TurnPlate_2_OP.h"
#include "RFID.h"

#define MOTOR_RESET				   0x03//电机复位
#define MOTOR_FREE				   0x04//电机释放
#define SET_TurnPlate_1_PLACE      0x11//设置转盘位置
#define READ_BARCODE			   0x12//读样本条码
#define OpenOrCloseScan_2		   0x13//打开/关闭流水线扫码枪
#define WRITE_TEST_MESSAGE		   0x14//写入测试信息到小车
#define CLEAR_CAR_MESSAGE		   0x18//清除小车信息
#define WRITE_BARCODE_TO_CAR	   0x19//写条码到小车
#define CLEAR_LIST				   0x1A//清除链表缓冲区
#define MOVE_TEST_TUBE    	   	   0x1B//移动试管
#define EMITYCAR_INTOTARK    	   0x1C//空小车是否进入轨道
#define SAVE_POINT				   0x07//读写定位参数
#define READ_WRITE_MOTOR_PARA	   0x08//读写电机参数

#define READ_BARCODE_TIMEOUT		4000//扫码超时


void TurnPlate_1_Module_ResetMotor(NetCmd *cmd)//转盘1_复位电机
{
	if(_State_Moudle.State_TurnPlate_1_Module == State_NoBusy)
	{
		_State_Moudle.State_TurnPlate_1_Module = State_Busy;

		//转盘复位
		if(UartSend(fd_RS485_index_1, TurnPlate_1_Mode_Motor_Add, MOTOR_COM_RESET, 0) != 0)
		{
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
			_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
		}
		TurnPlate_1_Module_Num = STEP_1;
	}else
	{
		if(TurnPlate_1_Module_Num == STEP_1)
		{
			//等转盘复位完成
			if(UartSend(fd_RS485_index_1, TurnPlate_1_Mode_Motor_Add, MOTOR_COM_READRESETSTATE, 0) != 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
				_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
			}else
			{
				if(PackByte(_ControlBoard[fd_RS485_index_1].Motor[TurnPlate_1_Mode_Motor_Add].point) == 0x01)//转盘复位完成
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);

				}else if(PackByte(_ControlBoard[fd_RS485_index_1].Motor[TurnPlate_1_Mode_Motor_Add].point) == 0x02)//转盘复位完成
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Reset_Err));
				}
				_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
			}

		}
	}
}

/*
 * 转盘移动
 * _steps 坐标
 *
 * return :
 * 1 : 通讯故障
 * 0 ：正常结束
 * 5 ： 运行中
 * 2 ： 碰撞
 *
 */
int TurnPlate_1_Move(char * _steps)
{
	if(_State_Moudle.State_TurnPlate_1_Module == State_NoBusy)
	{
		_State_Moudle.State_TurnPlate_1_Module = State_Busy;
		char _steps_buf[9] = {0};
		memcpy(_steps_buf, _steps, 8);
		if(UartSend(fd_RS485_index_1, TurnPlate_1_Mode_Motor_Add, MOTOR_COM_SETSTEPS,1, _steps_buf) != 0)
		{

			return 1;
		}
		TurnPlate_1_Module_Num = STEP_1;
	}else{
		if(TurnPlate_1_Module_Num == STEP_1)//等转盘移动完成
		{
			if(UartSend(fd_RS485_index_1, TurnPlate_1_Mode_Motor_Add, MOTOR_COM_READ_IN_PLACE, 0) != 0)
			{
				_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
				return 1;
			}
			if(PackByte(_ControlBoard[fd_RS485_index_1].Motor[TurnPlate_1_Mode_Motor_Add].point) != 0x00)//0为移动中
			{
				_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
				//判断状态
				if(PackByte(_ControlBoard[fd_RS485_index_1].Motor[TurnPlate_1_Mode_Motor_Add].point) == 0x02)//碰撞
				{
					return 2;
				}
				//完成
				return 0;
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
int TurnPlate_1_Read_Place()
{
	if(_State_Moudle.State_TurnPlate_1_Module == State_NoBusy)
	{
		_State_Moudle.State_TurnPlate_1_Module = State_Busy;
		if(UartSend(fd_RS485_index_1, TurnPlate_1_Mode_Motor_Add, MOTOR_COM_READSTEPS, 0) != 0)
		{
			_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
			return 1;
		}else
		{
			_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
			return 0;
		}
	}else
	{
		return 5;
	}
}

void TurnPlate_1_Module_Set_Place(NetCmd *cmd)
{
	if(AsciiToHex(cmd->pvar[0]) == 0x01)//设置位置
	{
		int ret = TurnPlate_1_Move(&cmd->pvar[1]);
		switch(ret)
		{
		case 0:
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			break;
		case 1:
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
			break;
		case 5:
			break;
		case 2://碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Collision));
			break;
		}
	}else if(AsciiToHex(cmd->pvar[0]) == 0x00)//读位置
	{
		int ret = TurnPlate_1_Read_Place();
		if(ret == 1)
		{
			//通讯失败
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
		}else
		{
			char _Point[16] = "";
			strncat(_Point, _ControlBoard[fd_RS485_index_1].Motor[TurnPlate_1_Mode_Motor_Add].point, 8);
			Eth_Send_Queue(cmd, 0, 0xFF, 2, 0000, _Point);
		}
	}
}

void TurnPlate_1_Module_LockMotor(NetCmd *cmd)//转盘1_锁定/释放电机
{
	if(_State_Moudle.State_TurnPlate_1_Module == State_NoBusy)
	{
		_State_Moudle.State_TurnPlate_1_Module = State_Busy;
		char Lock[2] = {0};
		Lock[0] = cmd->pvar[0];
		if(UartSend(fd_RS485_index_1, TurnPlate_1_Mode_Motor_Add, MOTOR_COM_FREE, 1, &Lock) != 0)
		{
			_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
			//报错
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
		}else
		{
			_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
		}
	}
}

/*
 * 移动试管
 */
void TurnPlate_1_Module_MoveTestTube(NetCmd *cmd)
{
	unsigned int Src_hole0x04 = PackByte((char*) &cmd->pvar[0]);
	unsigned int Des_hole0x04 = PackByte((char*) &cmd->pvar[2]);
	_Moudle_Point.TurnPlate_1_Module_point = turntableSamplePos->portPos[Src_hole0x04 + 1];
	char _point_buf[9] = {0};
	sprintf(_point_buf, "%08X", _Moudle_Point.TurnPlate_1_Module_point);
	static int a = 0;
	static struct timeval Time_start;
	static struct timeval Time_now;
	int err = 0;
	if(a == 0)
	{
		//运动到起始位置
		_Moudle_Point.TurnPlate_1_Module_point = turntableSamplePos->portPos[Src_hole0x04 - 1];
		sprintf(_point_buf, "%08X", _Moudle_Point.TurnPlate_1_Module_point);
		err = TurnPlate_1_Move(_point_buf);
		switch(err)
		{
		case 0:
			if(Src_hole0x04 == 2)//原位置== 2  不用延时
			{
				a = 2;
			}else
			{
				a = 1;
				gettimeofday(&Time_start, NULL);
			}
			break;
		case 1:
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
			break;
		case 5:
			break;
		case 2://碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Collision));
			break;
		}
	}
	if(a == 1)
	{
		//延时
		_State_Moudle.State_TurnPlate_1_Module = State_Busy;
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, turntableSamplePos->waitingTime[Src_hole0x04 / 2]) == 0)
		{
			a = 2;//超时完成
			_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
		}
	}
	if(a == 2)
	{
		//移动到目标位置
		_Moudle_Point.TurnPlate_1_Module_point = turntableSamplePos->portPos[Des_hole0x04 - 1];
		sprintf(_point_buf, "%08X", _Moudle_Point.TurnPlate_1_Module_point);
		err = TurnPlate_1_Move(_point_buf);
		switch(err)
		{
		case 0:
			if(Des_hole0x04 == 2)// 目标位置 == 2 不用延时
			{
				a = 0;//直接退出本条指令
				_Moudle_Point.TurnPlate_1_Module_NowHole = Des_hole0x04;
				Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			}else
			{
				a = 3;
				gettimeofday(&Time_start, NULL);
			}
			break;
		case 1:
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
			break;
		case 5:
			break;
		case 2://碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Collision));
			break;
		}
	}
	if(a == 3)
	{
		_State_Moudle.State_TurnPlate_1_Module = State_Busy;
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, turntableSamplePos->waitingTime[Des_hole0x04 / 2]) == 0)
		{
			a = 0;//超时完成
			_Moudle_Point.TurnPlate_1_Module_NowHole = Des_hole0x04;
			_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			Set_CarID_Can_Return(0);
		}
	}
}

/*
 * 打开或关闭扫码枪
 */
int TurnPlate_1_Module_OpenOrCloseScan(NetCmd *cmd, int flag)
{
	unsigned int Datanum[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	if(cmd->code == 0x13)
	{
		Datanum[0] = AsciiToHex(cmd->pvar[0]);
	}else
	{
		Datanum[0] = flag;
	}
	if(CanSendCommand(1, 0, OpenOrCloseScan_2_Command, Datanum) != 1)//发送不成功
	{
		_ControlBoard[5].Motor[0].State[OpenOrCloseScan_2_Command] = State_Error;
		//通讯失败
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_OPENBARCODE_Err));
		return -1;
	}
	if(cmd->code == 0x13)
	{
		//电机转
		if((AsciiToHex(cmd->pvar[0]) == 0) || (AsciiToHex(cmd->pvar[0]) == 2))
		{
			if(UartSend(fd_RS485_index_1, Rotate_TeatTube_Motor_Add, MOTOR_COM_MOMENT_MODE,1, "1") != 0)
			{
				//通信失败，报错
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_R_Motor_Communication_Err));
			}else
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, state1[_ControlBoard[5].Motor[0].State[OpenOrCloseScan_2_Command]]);
			}
		}else if(AsciiToHex(cmd->pvar[0]) == 1)
		{
			if(UartSend(fd_RS485_index_1, Rotate_TeatTube_Motor_Add, MOTOR_COM_MOMENT_MODE,1, "0") != 0)//电机停
			{
				//通信失败，报错
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_R_Motor_Communication_Err));
			}else
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, state1[_ControlBoard[5].Motor[0].State[OpenOrCloseScan_2_Command]]);
			}
		}
	}
	return 0;
}

/*
 * 读试管条码
 */
void TurnPlate_1_Module_Read_Bar_Code(NetCmd *cmd)
{
	static struct timeval Time_start;
	static struct timeval Time_now;
	if(_State_Moudle.State_TurnPlate_1_Module == State_NoBusy)
	{
		gettimeofday(&Time_start, NULL);
		_State_Moudle.State_TurnPlate_1_Module = State_Busy;
		//开扫码枪
		BarCode_recv_finish_2 = 0;//扫码完成标志
		if(TurnPlate_1_Module_OpenOrCloseScan(cmd, 2) != 0)
		{
			//和分控通信失败，报错
			_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_OPENBARCODE_Err));
		}else
		{
			//打开成功
			//转电机
			if(UartSend(fd_RS485_index_1, Rotate_TeatTube_Motor_Add, MOTOR_COM_MOMENT_MODE,1, "1") != 0)
			{
				//通信失败，报错
				_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_R_Motor_Communication_Err));
			}else
			{
				TurnPlate_1_Module_Num = STEP_1;
			}
		}
	}else
	{
		if(TurnPlate_1_Module_Num == STEP_1)
		{
			//等扫到码
			if(BarCode_recv_finish_2 == 1)
			{
				_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
				if(UartSend(fd_RS485_index_1, Rotate_TeatTube_Motor_Add, MOTOR_COM_MOMENT_MODE,1, "0") != 0)//停电机
				{
					//通信失败，报错
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_R_Motor_Communication_Err));
				}
				Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			}else
			{
				gettimeofday(&Time_now, NULL);
				if(My_timeout(&Time_start, &Time_now, READ_BARCODE_TIMEOUT) == 0)
				{
					_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
					if(TurnPlate_1_Module_OpenOrCloseScan(cmd, 1) != 0)//关闭扫码枪
					{
						Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_OPENBARCODE_Err));
					}
					if(UartSend(fd_RS485_index_1, Rotate_TeatTube_Motor_Add, MOTOR_COM_MOMENT_MODE,1, "0") != 0)//停电机
					{
						//通信失败，报错
						Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_R_Motor_Communication_Err));
					}
					//扫码超时，报错
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_SCANCODES_Err));
				}
			}
		}
	}
	//判断超时
		//超时关闭扫码枪、停电机
		//未超时、上返条码、停电机
}

void TurnPlate_1_Module_Write_BarCode_To_Car(NetCmd *cmd)
{
	int err = Write_RFID_Barcode(RFID_2_ADDR, &cmd->pvar[2],PackByte(&cmd->pvar[0]));
	if(err  == 0)
	{
		Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
	}else if(err  == 3)
	{
		//报错
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Load_RFID_Communication_Err));
	}else if(err  == 1)
	{
		//报错
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Load_RFID_FindCard_Err));
	}
}

//读写电机参数
void TurnPlate_1_Module_MotorPara(NetCmd *cmd)
{
	if(_State_Moudle.State_TurnPlate_1_Module == State_NoBusy)
	{
		_State_Moudle.State_TurnPlate_1_Module = State_Busy;
		if(AsciiToHex(cmd->pvar[0]) == 0)//读
		{
			if(UartSend(fd_RS485_index_1, PackByte(&cmd->pvar[1]), MOTOR_COM_READ_PARA, 0) != 0)
			{
				if(PackByte(&cmd->pvar[1]) == 0x01)//底部旋转电机
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
				}else
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_R_Motor_Communication_Err));
				}

			}else
			{
				char buf[128] = {0};

				int len = PackByte(_ControlBoard[fd_RS485_index_1].Motor[PackByte(&cmd->pvar[1])].point);
				if(len != 126)
				{

				}else
				{
					memcpy(buf, _ControlBoard[fd_RS485_index_1].Motor[PackByte(&cmd->pvar[1])].point + 2, 48);
					memcpy(buf + 48, _ControlBoard[fd_RS485_index_1].Motor[PackByte(&cmd->pvar[1])].point + 58, len - 56);
				}
				Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, buf);
			}
		}else if(AsciiToHex(cmd->pvar[0]) == 1)//写
		{
			char buf[128] = {0};
			int len = 118;
			memcpy(buf, &cmd->pvar[3], len);
			if(UartSend(fd_RS485_index_1, PackByte(&cmd->pvar[1]), MOTOR_COM_WRITE_PARA, 1, buf) != 0)
			{
				if(PackByte(&cmd->pvar[1]) == 0x01)//底部旋转电机
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
				}else
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_R_Motor_Communication_Err));
				}
			}else
			{
				//保存参数
				if(UartSend(fd_RS485_index_1, PackByte(&cmd->pvar[1]), MOTOR_COM_SAVE_PARA, 0) != 0)
				{
					if(PackByte(&cmd->pvar[1]) == 0x01)//底部旋转电机
					{
						Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
					}else
					{
						Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_R_Motor_Communication_Err));
					}
				}else
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, 0x0000);
				}
			}
		}else
		{

		}
		_State_Moudle.State_TurnPlate_1_Module = State_NoBusy;
	}
}


void TurnPlate_1_Module(NetCmd *cmd)
{
	switch(cmd->code){
	case READ_VERSION:
		switch(PackByte(&cmd->pvar[0])){
		case 0x01://靠近样品架的转盘
		case 0x02://扫码未扫上时转动的电机
			if(UartSend(fd_RS485_index_1, PackByte(&cmd->pvar[0]), MOTOR_COM_READ_VERSION, 0) != 0)
			{
				if(PackByte(&cmd->pvar[0]) == 1)//靠近样品架的转盘
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Communication_Err));
				}else//扫码未扫上时转动的电机
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_R_Motor_Communication_Err));
				}

			}else
			{
				char version[4][9] = {""};
				uint8_t len = PackByte(_ControlBoard[fd_RS485_index_1].Motor[PackByte(&cmd->pvar[0])].point);
				len += 2;
				int i = 0;
				int n = (len / 8) + (len % 8 == 0 ? 0 : 1);
				for(i = 0; i < n; i++)
				{
					if(i == n - 1)
					{
						if((len % 8) == 0)
						{
							memcpy(version[i],  _ControlBoard[fd_RS485_index_1].Motor[PackByte(&cmd->pvar[0])].point + (8 * i), 8);
						}else
						{
							memcpy(version[i],  _ControlBoard[fd_RS485_index_1].Motor[PackByte(&cmd->pvar[0])].point + (8 * i), len % 8);
							memset(version[i] + (len % 8), '0', 8 - (len % 8));
						}

					}else
					{
						memcpy(version[i],  _ControlBoard[fd_RS485_index_1].Motor[PackByte(&cmd->pvar[0])].point + (8 * i), 8);
					}
				}

				Eth_Send_Queue(cmd, 0, 0xFF, n + 1, 0x0000, version[0], version[1], version[2], version[3]);
			}
			break;
		case 0x03://卸载点RFID
			if(UartSend(fd_RS485_index_1, RFID_1_ADDR, MOTOR_COM_READ_VERSION, 0) != 0)
			{
				//有通讯故障，报错
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Unload_RFID_Communication_Err));
			}else
			{
				char version[4][9] = {""};
				uint8_t len = PackByte(_ControlBoard[fd_RS485_index_1].Motor[RFID_1_ADDR].point);
				len += 2;
				int i = 0;
				int n = (len / 8) + (len % 8 == 0 ? 0 : 1);
				for(i = 0; i < n; i++)
				{
					if(i == n - 1)
					{
						if((len % 8) == 0)
						{
							memcpy(version[i],  _ControlBoard[fd_RS485_index_1].Motor[RFID_1_ADDR].point + (8 * i), 8);
						}else
						{
							memcpy(version[i],  _ControlBoard[fd_RS485_index_1].Motor[RFID_1_ADDR].point + (8 * i), len % 8);
							memset(version[i] + (len % 8), '0', 8 - (len % 8));
						}

					}else
					{
						memcpy(version[i],  _ControlBoard[fd_RS485_index_1].Motor[RFID_1_ADDR].point + (8 * i), 8);
					}
				}

				Eth_Send_Queue(cmd, 0, 0xFF, n + 1, 0x0000, version[0], version[1], version[2], version[3]);
			}
			break;
		case 0x04://分控（扫码枪、蜂鸣器接在分控上）
		{
			unsigned int Datanum[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			if(CanSendCommand(1, 0, Read_Version,Datanum) != 1)//发送不成功
			{
				_ControlBoard[5].Motor[0].State[Read_Version] = State_Error;
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_OPENBARCODE_Err));
			}else
			{
				char version11[9] = "";
				char version12[9] = "";
				sprintf(version11, "%02X", 10);
				memcpy(version11 + 2, _ControlBoard[5].Motor[0].point, 6);
				memcpy(version12, _ControlBoard[5].Motor[0].point + 6, 4);
				strcat(version12, "0000");
				Eth_Send_Queue(cmd, 0, 0xFF, 3, 0x0000, version11, version12);
			}
		}
			break;
		}
		break;
	case MOTOR_RESET://复位
		TurnPlate_1_Module_ResetMotor(cmd);
		break;
	case SET_TurnPlate_1_PLACE://设置转盘位置
		TurnPlate_1_Module_Set_Place(cmd);
		break;
	case MOTOR_FREE://锁定/释放电机
		TurnPlate_1_Module_LockMotor(cmd);
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
			memcpy(turntableSamplePos, point, sizeof(SampleTurntablePos));
			SavePointConfigure(Filename_config, cmd);
			char send_buf0x07[16] = "";
			strncat(send_buf0x07, (char*)&cmd->pvar[2], 4);
			Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, send_buf0x07);
		}else
		{
			//读参数
			char s[1024] = "";
			ReadPointConfigure("POINT", "04", Filename_config, s);
			Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, s);
		}
		break;
	case MOVE_TEST_TUBE://移动试管
		TurnPlate_1_Module_MoveTestTube(cmd);
		break;
	case READ_BARCODE://读试管条码
		TurnPlate_1_Module_Read_Bar_Code(cmd);
		break;
	case OpenOrCloseScan_2:////打开/关闭流水线扫码枪
		TurnPlate_1_Module_OpenOrCloseScan(cmd, 0);
		break;
	case EMITYCAR_INTOTARK:////空小车是否进入轨道
		Set_EmptyCar_IntoTark(AsciiToHex(cmd->pvar[0]));
		Eth_Send_Queue(cmd, 0, 0xFF, 1, 0x0000);
		break;
	case CLEAR_LIST://清除链表缓冲区
		ClearList(&list);
		Eth_Send_Queue(cmd, 0, 0xFF, 1, 0x0000);
		break;
	case CLEAR_CAR_MESSAGE://清除小车信息
		if(clear_car_tarck_or_message(RFID_1_ADDR, AsciiToHex(cmd->pvar[0]), Pack8Byte(&cmd->pvar[1])) == 0)
		{
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0x0000);
		}else
		{
			//报错
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(TurnPlate_1_ModuleCommandAdd, TurnPlate_1_Module_Unload_RFID_Communication_Err));
		}
		break;
	case WRITE_BARCODE_TO_CAR:
		TurnPlate_1_Module_Write_BarCode_To_Car(cmd);
		break;
	case READ_WRITE_MOTOR_PARA://读写电机参数
		TurnPlate_1_Module_MotorPara(cmd);
		break;
	}
}


