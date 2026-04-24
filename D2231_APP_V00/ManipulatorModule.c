/*
 * ManipulatorModule.c
 *
 *  Created on: 2025-12-24
 *      Author: ThinkPad
 */
#include "ManipulatorModule.h"


#define SET_MAINIPULATOR_PLACE     0x11//设置机械手位置
#define READ_MAINIPULATOR_PLACE    0x12//读机械手位置
#define MOVE_TEST_TUBE    	   	   0x13//移动试管
#define MOVE_TO_TARGET_AREA    	   0x14//移动到指定区域
#define READ_SET_HAND_PLACE    	   0x15//读电爪位置
#define READ_SAMPLESHELF_BARCODE   0x16//读样品架条码
#define OpenOrCloseScan			   0x17//打开/关闭扫码枪
#define MOTOR_FREE				   0x04//电机释放
#define MOTOR_RESET				   0x03//电机复位
#define SAVE_POINT				   0x07//读写定位参数
#define READ_WRITE_MOTOR_PARA	   0x08//读写电机参数

#define READ_SHELF_BARCODE_TIMEOUT		1000//扫码超时
/*
 * 机械手移动
 * x_steps x轴坐标
 * y_steps y轴坐标
 * z_steps z轴坐标
 * hand_steps 爪子动作 0-无动作，1-夹住，2-放开
 * Preparatory_actions 前置动作，是否需要抬起Z轴和松开爪子
 * x_flag x轴是否移动  0：不动， 1：动
 * y_flag y轴是否移动
 * z_flag z轴是否移动
 *
 * return :
 * 4 : 爪子通讯故障
 * 3 : Z轴通讯故障
 * 2 : y轴通讯故障
 * 1 : x轴通讯故障
 * 0 ：正常结束
 * 5 ： 运行中
 * 6 ： 未夹到试管
 * 7 : 松开爪子成功
 * 8 : Z轴碰撞
 * 9 : x轴碰撞
 * 10: y轴碰撞
 *
 */
int ManipulatorMove(char * x_steps, char * y_steps, char * z_steps, char *hand_steps, int Preparatory_actions,  int x_flag, int y_flag, int z_flag)
{
	static struct timeval Time_start;
	static struct timeval Time_now;
	if(_State_Moudle.State_Manipulator == State_NoBusy)
	{
		_State_Moudle.State_Manipulator = State_Busy;
		ManipulatorModule_Num = STEP_1;

	}else
	{

		if(ManipulatorModule_Num == STEP_1)
		{
			if(Preparatory_actions  > 0)//需要先抬起Z轴和松开爪子
			{
				//读爪子状态
				if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, HAND_COM_READSTATE, 0) != 0)
				{
					//有通讯故障，报错
					_State_Moudle.State_Manipulator = State_NoBusy;
					return 4;

				}else{
					//判断爪子状态
					switch(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point))
					{
					case 0x01://夹到
					case 0x03://没有夹持到且大于设定极限03,夹持到且丢了也是03
						//松开后抬起Z轴
						ManipulatorModule_Num = STEP_2;
						break;
					case 0x04://张开或者正在夹,正在运行状态04
						//继续读爪子状态
						break;
					case 0x05://张开完成05
						//抬起Z轴
						ManipulatorModule_Num = STEP_3;
						break;
					}
				}
			}else
			{
				ManipulatorModule_Num = STEP_4;
			}
		}else if(ManipulatorModule_Num == STEP_2)//发送松开爪子指令
		{

			//松开爪子
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, HAND_COM_MOVEMENT,1, HAND_COM_MOVEMENT_PUTDOWN) != 0)
			{
				//有通讯故障，报错
				_State_Moudle.State_Manipulator = State_NoBusy;
				return 4;
			}else{
				ManipulatorModule_Num = STEP_1;//返回上一步，继续读爪子状态
			}
		}else if(ManipulatorModule_Num == STEP_3)//抬起Z轴
		{
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_SETSTEPS,1, MOTOR_0_STEPS) != 0)
			{
				//有通讯故障，报错
				_State_Moudle.State_Manipulator = State_NoBusy;
				return 3;
			}else
			{
				ManipulatorModule_Num = STEP_4;
			}
		}else if(ManipulatorModule_Num == STEP_4)//等抬起Z轴完成，移动X/Y
		{
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_READ_IN_PLACE, 0) != 0)
			{
				//有通讯故障，报错
				_State_Moudle.State_Manipulator = State_NoBusy;
				return 3;
			}else
			{
				if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_z].point) != 0x00)//0为移动中
				{
					//判断运动状态是否有故障
					if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_z].point) == 0x02)//碰撞
					{
						_State_Moudle.State_Manipulator = State_NoBusy;
						return 8;
					}
					//移动X/Y

					if(x_flag)//X需要移动
					{
						char x_steps_buf[9] = {0};
						memcpy(x_steps_buf, x_steps, 8);
						if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_x, MOTOR_COM_SETSTEPS,1, x_steps_buf) != 0)
						{
							_State_Moudle.State_Manipulator = State_NoBusy;
							return 1;
						}
					}
					if(y_flag)//y需要移动
					{
						char y_steps_buf[9] = {0};
						memcpy(y_steps_buf, y_steps, 8);
						if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_y, MOTOR_COM_SETSTEPS,1, y_steps_buf) != 0)
						{
							_State_Moudle.State_Manipulator = State_NoBusy;
							return 2;
						}
					}
					ManipulatorModule_Num = STEP_5;
				}

			}
		}else if(ManipulatorModule_Num == STEP_5)//等X/Y轴完成
		{
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_x, MOTOR_COM_READ_IN_PLACE, 0) != 0)
				return 1;
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_y, MOTOR_COM_READ_IN_PLACE, 0) != 0)
				return 2;
			if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_x].point) != 0x00
					&& PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_y].point) != 0x00)//0为移动中
			{
				//判断运动状态是否有故障
				if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_x].point) == 0x02)//碰撞
				{
					_State_Moudle.State_Manipulator = State_NoBusy;
					return 9;
				}
				if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_y].point) == 0x02)//碰撞
				{
					_State_Moudle.State_Manipulator = State_NoBusy;
					return 10;
				}

				//移动Z轴
				if(z_flag)//Z需要移动
				{
					char z_steps_buf[9] = {0};
					memcpy(z_steps_buf, z_steps, 8);
					if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_SETSTEPS,1, z_steps_buf) != 0)
					{
						_State_Moudle.State_Manipulator = State_NoBusy;
						return 3;
					}
				}
				ManipulatorModule_Num = STEP_6;
			}
		}else if(ManipulatorModule_Num == STEP_6)//等z轴完成
		{
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_READ_IN_PLACE, 0) != 0)
			{
				//有通讯故障，报错
				_State_Moudle.State_Manipulator = State_NoBusy;
				return 3;
			}else
			{
				if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_z].point) != 0x00)//0为移动中
				{
					//判断运动状态是否有故障
					if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_z].point) == 0x02)//碰撞
					{
						_State_Moudle.State_Manipulator = State_NoBusy;
						return 8;
					}

					//电爪动作
					if(AsciiToHex(*hand_steps) == 1 || AsciiToHex(*hand_steps) == 2)//电爪需要移动
					{
						char hand_movement[8] = {0};
						memcpy(hand_movement, hand_steps, 1);
						if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, HAND_COM_MOVEMENT,1, hand_movement) != 0)
						{
							_State_Moudle.State_Manipulator = State_NoBusy;
							return 4;
						}
						ManipulatorModule_Num = STEP_7;
					}else
					{
						_State_Moudle.State_Manipulator = State_NoBusy;
						return 0;
					}

				}
			}
		}else if(ManipulatorModule_Num == STEP_7)//等电爪完成
		{
			//读爪子状态
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, HAND_COM_READSTATE, 0) != 0)
			{
				//有通讯故障，报错
				_State_Moudle.State_Manipulator = State_NoBusy;
				return 4;
			}else{
				//判断爪子状态
				if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point) != 0x04)//张开或者正在夹,正在运行状态04
				{
					_State_Moudle.State_Manipulator = State_NoBusy;
					if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point) == 0x01)
					{
						_State_Moudle.State_Manipulator = State_NoBusy;
						return 0;
					}else if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point) == 0x03)
					{
						_State_Moudle.State_Manipulator = State_NoBusy;
						return 6;
					}else if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point) == 0x05)
					{
						_State_Moudle.State_Manipulator = State_NoBusy;
						return 7;
					}

					//以下没用
					gettimeofday(&Time_start, NULL);
					ManipulatorModule_Num = STEP_8;

				}
			}
		}else if(ManipulatorModule_Num == STEP_8)//电爪完成后延时100ms
		{
			gettimeofday(&Time_now, NULL);
			if(My_timeout(&Time_start, &Time_now, 5000) == 0)
			{
				_State_Moudle.State_Manipulator = State_NoBusy;
				return 0;
			}
		}
	}
	return 5;
}

void ManipulatorModule_Set_Place(NetCmd *cmd)
{
	int ret = ManipulatorMove(&cmd->pvar[2], &cmd->pvar[10], &cmd->pvar[18], &cmd->pvar[27], 1, BIT(PackByte(&cmd->pvar[0]), 0), BIT(PackByte(&cmd->pvar[0]), 1), BIT(PackByte(&cmd->pvar[0]), 2));
	switch(ret){
	case 0://
		Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
		break;
	case 1://x轴通讯故障
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
		break;
	case 2://y轴通讯故障
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
		break;
	case 3://Z轴通讯故障
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
		break;
	case 4://爪子通讯故障
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
		break;
	case 5://运行中
		break;
	case 7://松开爪子成功
		Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
		break;
	case 6://未夹到试管
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
		break;
	case 8://Z轴碰撞
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
		break;
	case 9://x轴碰撞
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
		break;
	case 10://y轴碰撞
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
		break;
	}


}

/*
 * 读电机状态（其中会判断电机是否在运行中，如果在运行中才会去读状态）
 * 在Read_Motor_State()中调用
 */
void ManipulatorModule_Read_Motor_State()
{

}

void ManipulatorModule_ResetMotor(NetCmd *cmd)//机械手_复位电机
{
	if(_State_Moudle.State_Manipulator == State_NoBusy)
	{
		_State_Moudle.State_Manipulator = State_Busy;

		//电爪复位
		if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, MOTOR_COM_RESET, 0) != 0)
		{
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			_State_Moudle.State_Manipulator = State_NoBusy;
		}
		ManipulatorModule_Num = STEP_1;
	}else
	{
		if(ManipulatorModule_Num == STEP_1)
		{
			//查电爪复位状态，等待复位完成
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, MOTOR_COM_READRESETSTATE, 0) != 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
				_State_Moudle.State_Manipulator = State_NoBusy;
			}else
			{
				if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point) == 0x01)//电爪复位完成，复位Z轴
				{
					if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_RESET, 0) != 0)
					{
						Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
						_State_Moudle.State_Manipulator = State_NoBusy;
					}else
					{
						ManipulatorModule_Num = STEP_2;
					}
				}else if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point) == 0x02)//电爪复位失败
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Reset_Err));
					_State_Moudle.State_Manipulator = State_NoBusy;
				}

			}

		}else if(ManipulatorModule_Num == STEP_2)
		{
			//查询Z轴复位状态
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_READRESETSTATE, 0) != 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
				_State_Moudle.State_Manipulator = State_NoBusy;
			}else
			{
				if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_z].point) == 0x01)//Z轴复位完成，复位其他
				{
					if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_x, MOTOR_COM_RESET, 0) != 0)
					{
						Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
						_State_Moudle.State_Manipulator = State_NoBusy;
					}else
					{
						if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_y, MOTOR_COM_RESET, 0) != 0)
						{
							Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
							_State_Moudle.State_Manipulator = State_NoBusy;
						}else
						{
							ManipulatorModule_Num = STEP_3;
						}
					}

				}else if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_z].point) == 0x02)//Z轴复位失败
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Reset_Err));
					_State_Moudle.State_Manipulator = State_NoBusy;
				}
			}

		}else if(ManipulatorModule_Num == STEP_3)
		{
			//查询其他轴复位状态
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_x, MOTOR_COM_READRESETSTATE, 0) != 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
				_State_Moudle.State_Manipulator = State_NoBusy;
			}else
			{
				if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_y, MOTOR_COM_READRESETSTATE, 0) != 0)
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
					_State_Moudle.State_Manipulator = State_NoBusy;
				}else
				{
					if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_x].point) == 0x01
							&& PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_y].point) == 0x01)//其他轴复位完成，复位完成
					{
						Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
						_State_Moudle.State_Manipulator = State_NoBusy;
					}else if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_x].point) == 0x02)//x轴复位失败
					{
						Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Reset_Err));
						_State_Moudle.State_Manipulator = State_NoBusy;
					}else if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_y].point) == 0x02)//y轴复位失败
					{
						Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Reset_Err));
						_State_Moudle.State_Manipulator = State_NoBusy;
					}
				}

			}
		}
	}
}

void ManipulatorModule_LockMotor(NetCmd *cmd)//机械手_锁定/释放电机
{
	if(_State_Moudle.State_Manipulator == State_NoBusy)
	{
		_State_Moudle.State_Manipulator = State_Busy;
		int err = 0;
		char Lock = 0;
		Lock = cmd->pvar[0];
		if(BIT(PackByte((char*) &cmd->pvar[1]), 0) == 1)//X轴
		{
			err = UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_x, MOTOR_COM_FREE, 1, &Lock);
		}
		if(BIT(PackByte((char*) &cmd->pvar[1]), 1) == 1)//y轴
		{
			err += UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_y, MOTOR_COM_FREE, 1,  &Lock) << 1;
		}
		if(BIT(PackByte((char*) &cmd->pvar[1]), 2) == 1)//z轴
		{
			err += UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_FREE, 1,  &Lock) << 2;
		}
		if(BIT(PackByte((char*) &cmd->pvar[1]), 3) == 1)//爪子
		{
			err += UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, MOTOR_COM_FREE, 1,  &Lock) << 3;
		}
		if(0 != err)
		{
			if(BIT(err, 0) > 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			}
			else if(BIT(err, 1) > 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			}
			else if(BIT(err, 2) > 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			}else if(BIT(err, 3) > 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			}
		}else
		{
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
		}
		_State_Moudle.State_Manipulator = State_NoBusy;
	}
}

void ManipulatorModule_ReadSteps(NetCmd *cmd)//机械手_读取电机位置
{
	char xPoint[16] = "";
	char yPoint[16] = "";
	char zPoint[16] = "";
	char handState[16] = "";
	if(_State_Moudle.State_Manipulator == State_NoBusy)
	{
		_State_Moudle.State_Manipulator = State_Busy;
		int err = 0;
		err = UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_x, MOTOR_COM_READSTEPS, 0);
		err += UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_y, MOTOR_COM_READSTEPS, 0) << 1;
		err += UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_z, MOTOR_COM_READSTEPS, 0) << 2;
		err += UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, HAND_COM_READSTATE, 0) << 3;
		if(0 != err)
		{
			//有通讯故障，报错
			if(BIT(err, 0) > 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			}
			else if(BIT(err, 1) > 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			}
			else if(BIT(err, 2) > 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			}else if(BIT(err, 3) > 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			}
		}else
		{
			strncat(xPoint, _ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_x].point, 8);
			strncat(yPoint, _ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_y].point, 8);
			strncat(zPoint, _ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_z].point, 8);
			strncat(handState, _ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point, 2);
			sprintf(handState, "%s000000", handState);
			Eth_Send_Queue(cmd, 0, 0xFF, 5, 0000, xPoint, yPoint, zPoint, handState);
		}
		_State_Moudle.State_Manipulator = State_NoBusy;
	}
}

void ManipulatorModule_Hand_Read_Set_Place(NetCmd *cmd)
{
	if(_State_Moudle.State_Manipulator == State_NoBusy)
	{
		_State_Moudle.State_Manipulator = State_Busy;
		if(AsciiToHex(cmd->pvar[0]) == 0)//读
		{
			char handPoint[16] = "";
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, HAND_COM_READSTEPS, 0) != 0)
			{
				//有通讯故障，报错
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			}else
			{
				strncat(handPoint, _ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point, 8);
				Eth_Send_Queue(cmd, 0, 0xFF, 2, 0000,  handPoint);
			}
			_State_Moudle.State_Manipulator = State_NoBusy;
		}else if(AsciiToHex(cmd->pvar[0]) == 1)//写
		{
			char hand_steps[8] = {0};
			memcpy(hand_steps, &cmd->pvar[1], 8);
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, MOTOR_COM_SETSTEPS,1, hand_steps) != 0)
			{
				//有通讯故障，报错
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
				_State_Moudle.State_Manipulator = State_NoBusy;
			}else
			{
				ManipulatorModule_Num = STEP_1;
			}
		}
	}else
	{
		if(ManipulatorModule_Num == STEP_1)//读电机运行状态，判断是否到位
		{
			//读爪子状态
			if(UartSend(fd_RS485_index_2, Manipulator_Mode_Motor_Add_hand, HAND_COM_READSTATE, 0) != 0)
			{
				//有通讯故障，报错
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
				_State_Moudle.State_Manipulator = State_NoBusy;
			}else{
				//判断爪子状态
				if(PackByte(_ControlBoard[fd_RS485_index_2].Motor[Manipulator_Mode_Motor_Add_hand].point) != 0x04)//张开或者正在夹,正在运行状态04
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
					_State_Moudle.State_Manipulator = State_NoBusy;
				}
			}
		}
	}
}

void ManipulatorModule_Move_To_Target_Aera(NetCmd *cmd)
{
	unsigned int Area0x02 = PackWord((char*) &cmd->pvar[0]);
	unsigned int Hole0x02 = PackWord((char*) &cmd->pvar[4]);
	switch(Area0x02){
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
		_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->sampleAreaPos.clawRackPos[Area0x02 - 1].startBasePos.x + (((Hole0x02 - 1) % 5) * My_ClawsPos->sampleAreaPos.interval.xHoleInterval);
		_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->sampleAreaPos.clawRackPos[Area0x02 - 1].startBasePos.y + (((Hole0x02 - 1) / 5) * My_ClawsPos->sampleAreaPos.interval.yHoleInterval);
		_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->sampleAreaPos.clawRackPos[Area0x02 - 1].startBasePos.z;
		break;
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0A:
		_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->emergencyAreaPos.startBasePos[Area0x02 - 7].x ;
		_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->emergencyAreaPos.startBasePos[Area0x02 - 7].y + (((Hole0x02 - 1) ) * My_ClawsPos->emergencyAreaPos.interval.yHoleInterval);
		_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->emergencyAreaPos.startBasePos[Area0x02 - 7].z;
		break;
	case 0x0B://转盘1
		if(Hole0x02 == 2)//转盘加载试管口
		{
			_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->enterTrackPos.x;
			_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->enterTrackPos.y;
			_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->enterTrackPos.z;
		}else if(Hole0x02 == 3)//转盘卸载试管口
		{
			_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->unloadTrackPos.x;
			_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->unloadTrackPos.y;
			_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->unloadTrackPos.z;
		}
		break;
	}
	char x_point_buf[9] = {0};
	char y_point_buf[9] = {0};
	char z_point_buf[9] = {0};
	sprintf(x_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_x);
	sprintf(y_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_y);
	sprintf(z_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_z);
	int  z_flag = 0;
	if(AsciiToHex(cmd->pvar[9]) == 1 || AsciiToHex(cmd->pvar[9]) == 2)
	{
		z_flag = 1;
	}
	static int a = 0;
	//运行到目标位置
	if(a == 0)
	{
		int ret = ManipulatorMove(x_point_buf, y_point_buf, z_point_buf,
				&cmd->pvar[9], AsciiToHex(cmd->pvar[8]), 1, 1, z_flag);
		switch(ret){
		case 0:
			//Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			a = 1;
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		case 7://松开爪子成功
			a = 1;
			break;
		case 6://未夹到试管
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
			break;
		case 8://Z轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
			break;
		case 9://x轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
			break;
		case 10://y轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
			break;
		}
	}
	if(a == 1)
	{
		int ret = ManipulatorMove(x_point_buf, y_point_buf, MOTOR_0_STEPS,
				"00", 0, 0, 0, z_flag);
		switch(ret){
		case 0:
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			a = 0;
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		case 7://松开爪子成功
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			break;
		case 6://未夹到试管
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
			break;
		case 8://Z轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
			break;
		case 9://x轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
			break;
		case 10://y轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
			break;
		}
	}
}

/*
 * 打开或关闭扫码枪
 */
int ManipulatorModule_OpenOrCloseScan(NetCmd *cmd, int flag)
{
	unsigned int Datanum[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	if(cmd->code == 0x17)
	{
		Datanum[0] = AsciiToHex(cmd->pvar[0]);
	}else
	{
		Datanum[0] = flag;
	}
	if(CanSendCommand(1, 0, OpenOrCloseScan_Command, Datanum) != 1)//发送不成功
	{
		_ControlBoard[5].Motor[0].State[OpenOrCloseScan_Command] = State_Error;
		//通讯失败
		return -1;
	}
	if(cmd->code == 0x17)
	{
		Eth_Send_Queue(cmd, 0, 0xFF, 1, state1[_ControlBoard[5].Motor[0].State[OpenOrCloseScan_Command]]);
	}
	return 0;
}


void ManipulatorModule_Read_SampleShelf_BarCode(NetCmd *cmd)
{
	_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->sampleAreaPos.clawRackPos[AsciiToHex(cmd->pvar[0]) - 1].scanFrameCodePos.x;
	_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->sampleAreaPos.clawRackPos[AsciiToHex(cmd->pvar[0]) - 1].scanFrameCodePos.y;
	char x_point_buf[9] = {0};
	char y_point_buf[9] = {0};
	sprintf(x_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_x);
	sprintf(y_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_y);
	static int a = 0;

	static struct timeval Time_start;
	static struct timeval Time_now;

	if(a == 0)//移动到目标位置
	{
		int ret = ManipulatorMove(x_point_buf, y_point_buf, MOTOR_0_STEPS,
				"00", 1, 1, 1, 0);
		switch(ret){
		case 0:
			a = 1;
			Set_ManipulatorModule_Area(AsciiToHex(cmd->pvar[0]));
			_State_Moudle.State_Manipulator = State_Busy;
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		}
	}
	if(a == 1)//打开扫码枪
	{
		BarCode_recv_finish_1 = 0;//扫码完成标志
		gettimeofday(&Time_start, NULL);
		if(ManipulatorModule_OpenOrCloseScan(cmd, 2) == 0)
		{
			a = 2;
		}else
		{
			//报错 打开扫码枪失败
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_OPENBARCODE_Err));
			a = 0;
		}
	}
	if(a == 2)//等扫到码
	{
		if(BarCode_recv_finish_1 == 1)
		{
			_State_Moudle.State_Manipulator = State_NoBusy;
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			a = 0;
		}else
		{
			gettimeofday(&Time_now, NULL);
			if(My_timeout(&Time_start, &Time_now, READ_SHELF_BARCODE_TIMEOUT) == 0)
			{
				if(ManipulatorModule_OpenOrCloseScan(cmd, 1) == 0)//关闭扫码枪
				{
					a = 0;
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_SCANCODES_Err));
					_State_Moudle.State_Manipulator = State_NoBusy;
				}else
				{
					//报错 关闭扫码枪失败
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_OPENBARCODE_Err));
					a = 0;
				}
			}
		}
	}
}

void ManipulatorModule_MoveTestTube(NetCmd *cmd)//移动试管
{
	unsigned int Area0x02 = PackWord((char*) &cmd->pvar[0]);
	unsigned int Hole0x02 = PackWord((char*) &cmd->pvar[4]);
	switch(Area0x02){
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
		_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->sampleAreaPos.clawRackPos[Area0x02 - 1].startBasePos.x + (((Hole0x02 - 1) % 5) * My_ClawsPos->sampleAreaPos.interval.xHoleInterval);
		_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->sampleAreaPos.clawRackPos[Area0x02 - 1].startBasePos.y + (((Hole0x02 - 1) / 5) * My_ClawsPos->sampleAreaPos.interval.yHoleInterval);
		_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->sampleAreaPos.clawRackPos[Area0x02 - 1].startBasePos.z;
		break;
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0A:
		_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->emergencyAreaPos.startBasePos[Area0x02 - 7].x ;
		_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->emergencyAreaPos.startBasePos[Area0x02 - 7].y + (((Hole0x02 - 1) ) * My_ClawsPos->emergencyAreaPos.interval.yHoleInterval);
		_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->emergencyAreaPos.startBasePos[Area0x02 - 7].z;
		break;
	case 0x0B://转盘1
		if(Hole0x02 == 2)//转盘加载试管口
		{
			_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->enterTrackPos.x;
			_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->enterTrackPos.y;
			_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->enterTrackPos.z;
		}else if(Hole0x02 == 3)//转盘卸载试管口
		{
			_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->unloadTrackPos.x;
			_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->unloadTrackPos.y;
			_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->unloadTrackPos.z;
		}
		break;
	}
	char x_point_buf[9] = {0};
	char y_point_buf[9] = {0};
	char z_point_buf[9] = {0};
	sprintf(x_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_x);
	sprintf(y_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_y);
	sprintf(z_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_z);
	static int a = 0;
	if(a == 0)//松开爪子，抬起Z轴
	{
		int ret = ManipulatorMove(x_point_buf, y_point_buf, MOTOR_0_STEPS,
				"00", 1, 0, 0, 1);
		switch(ret){
		case 0:
			a = 1;
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		case 7://松开爪子成功
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			break;
		case 6://未夹到试管
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
			break;
		case 8://Z轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
			break;
		case 9://x轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
			break;
		case 10://y轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
			break;
		}
	}
	if(a == 1)//到目标位置
	{
		int ret = ManipulatorMove(x_point_buf, y_point_buf, z_point_buf,
				"00", 0, 1, 1, 0);
		switch(ret){
		case 0:
			//Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			a = 2;
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		case 7://松开爪子成功
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			break;
		case 6://未夹到试管
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
			break;
		case 8://Z轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
			break;
		case 9://x轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
			break;
		case 10://y轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
			break;
		}
	}
	if(a == 2)//Z轴下探，试管夹住
	{
		//如果在转盘位置需等转盘到位
		int ret = ManipulatorMove(x_point_buf, y_point_buf, z_point_buf,
				"1", 0, 0, 0, 1);
		switch(ret){
		case 0:
			//Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			a = 3;
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		case 7://松开爪子成功
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			break;
		case 6://未夹到试管
			a = 0;
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
			break;
		case 8://Z轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
			break;
		case 9://x轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
			break;
		case 10://y轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
			break;
		}
	}
	if(a == 3)//抬起Z轴
	{
		int ret = ManipulatorMove(x_point_buf, y_point_buf, MOTOR_0_STEPS,
				"00", 0, 0, 0, 1);
		switch(ret){
		case 0:
			a = 4;
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		case 7://松开爪子成功
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			break;
		case 6://未夹到试管
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
			break;
		case 8://Z轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
			break;
		case 9://x轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
			break;
		case 10://y轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
			break;
		}
	}
	unsigned int DstArea0x02 = PackWord((char*) &cmd->pvar[8]);
	unsigned int DstHole0x02 = PackWord((char*) &cmd->pvar[12]);
	switch(DstArea0x02){
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
		_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->sampleAreaPos.clawRackPos[DstArea0x02 - 1].startBasePos.x + (((DstHole0x02 - 1) % 5) * My_ClawsPos->sampleAreaPos.interval.xHoleInterval);
		_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->sampleAreaPos.clawRackPos[DstArea0x02 - 1].startBasePos.y + (((DstHole0x02 - 1) / 5) * My_ClawsPos->sampleAreaPos.interval.yHoleInterval);
		_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->sampleAreaPos.clawRackPos[DstArea0x02 - 1].startBasePos.z;
		break;
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0A:
		_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->emergencyAreaPos.startBasePos[DstArea0x02 - 7].x ;
		_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->emergencyAreaPos.startBasePos[DstArea0x02 - 7].y + (((DstHole0x02 - 1) ) * My_ClawsPos->emergencyAreaPos.interval.yHoleInterval);
		_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->emergencyAreaPos.startBasePos[DstArea0x02 - 7].z;
		break;
	case 0x0B://转盘1
		if(DstHole0x02 == 2)//转盘加载试管口
		{
			_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->enterTrackPos.x;
			_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->enterTrackPos.y;
			_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->enterTrackPos.z;
		}else if(DstHole0x02 == 3)//转盘卸载试管口
		{
			_Moudle_Point.ManipulatorModule_point_x = My_ClawsPos->unloadTrackPos.x;
			_Moudle_Point.ManipulatorModule_point_y = My_ClawsPos->unloadTrackPos.y;
			_Moudle_Point.ManipulatorModule_point_z = My_ClawsPos->unloadTrackPos.z;
		}
		break;
	}
	sprintf(x_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_x);
	sprintf(y_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_y);
	sprintf(z_point_buf, "%08X", _Moudle_Point.ManipulatorModule_point_z);
	if(a == 4)//运行到目标位置
	{
		int ret = ManipulatorMove(x_point_buf, y_point_buf, z_point_buf,
				"00", 0, 1, 1, 0);
		switch(ret){
		case 0:
			//Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			a = 5;
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		case 7://松开爪子成功
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			break;
		case 6://未夹到试管
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
			break;
		case 8://Z轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
			break;
		case 9://x轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
			break;
		case 10://y轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
			break;
		}
	}
	if(a == 5)
	{
		//如果在转盘位置需等转盘到位

		if((DstHole0x02 == 2) && (DstArea0x02 == 0x0B))
		{
			if(_Moudle_Point.TurnPlate_1_Module_NowHole != 2)
			{
				_State_Moudle.State_Manipulator = State_Busy;
				return;
			}else
			{
				_State_Moudle.State_Manipulator = State_NoBusy;
				a = 6;
			}
		}else
		{
			a = 6;
		}
	}
	if(a == 6)//Z轴下探，松开爪子
	{
		int ret = ManipulatorMove(x_point_buf, y_point_buf, z_point_buf,
				"2", 0, 0, 0, 1);
		switch(ret){
		case 0:
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		case 7://松开爪子成功
			a = 7;
			break;
		case 6://未夹到试管
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
			break;
		case 8://Z轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
			break;
		case 9://x轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
			break;
		case 10://y轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
			break;
		}
	}
	if(a == 7)//抬起Z轴
	{
		int ret = ManipulatorMove(x_point_buf, y_point_buf, MOTOR_0_STEPS,
				"00", 0, 0, 0, 1);
		switch(ret){
		case 0:
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			a = 0;
			break;
		case 1://x轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Communication_Err));
			break;
		case 2://y轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Communication_Err));
			break;
		case 3://Z轴通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Communication_Err));
			break;
		case 4://爪子通讯故障
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Communication_Err));
			break;
		case 5://运行中
			break;
		case 7://松开爪子成功
			Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
			break;
		case 6://未夹到试管
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_hand - 1].Collision));
			break;
		case 8://Z轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_z - 1].Collision));
			break;
		case 9://x轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_x - 1].Collision));
			break;
		case 10://y轴碰撞
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[Manipulator_Mode_Motor_Add_y - 1].Collision));
			break;
		}
	}
}

//读写电机参数
void ManipulatorModule_MotorPara(NetCmd *cmd)
{
	if(_State_Moudle.State_Manipulator == State_NoBusy)
	{
		_State_Moudle.State_Manipulator = State_Busy;
		if(AsciiToHex(cmd->pvar[0]) == 0)//读
		{
			if(UartSend(fd_RS485_index_2, PackByte(&cmd->pvar[1]), MOTOR_COM_READ_PARA, 0) != 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[PackByte(&cmd->pvar[1]) - 1].Communication_Err));
			}else
			{
				char buf[128] = {0};
				if(PackByte(&cmd->pvar[1]) == 4)//电爪
				{
					int len = PackByte(_ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[1])].point);
					if(len != 88)
					{

					}else
					{
						memcpy(buf, _ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[1])].point + 2, 32);
						memcpy(buf + 32, _ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[1])].point + 42, len - 40);
					}

				}else
				{
					int len = PackByte(_ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[1])].point);
					if(len != 126)
					{

					}else
					{
						memcpy(buf, _ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[1])].point + 2, 48);
						memcpy(buf + 48, _ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[1])].point + 58, len - 56);
					}
				}
				Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, buf);
			}
		}else if(AsciiToHex(cmd->pvar[0]) == 1)//写
		{
			char buf[128] = {0};
			int len = 0;
			if(PackByte(&cmd->pvar[1]) == 4)//电爪
			{
				len = 80;
			}else
			{
				len = 118;
			}
			memcpy(buf, &cmd->pvar[3], len);
			if(UartSend(fd_RS485_index_2, PackByte(&cmd->pvar[1]), MOTOR_COM_WRITE_PARA, 1, buf) != 0)
			{
				Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[PackByte(&cmd->pvar[1]) - 1].Communication_Err));
			}else
			{
				//保存参数
				if(UartSend(fd_RS485_index_2, PackByte(&cmd->pvar[1]), MOTOR_COM_SAVE_PARA, 0) != 0)
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[PackByte(&cmd->pvar[1]) - 1].Communication_Err));
				}else
				{
					Eth_Send_Queue(cmd, 0, 0xFF, 1, 0x0000);
				}
			}

		}else
		{

		}
		_State_Moudle.State_Manipulator = State_NoBusy;
	}
}

void ManipulatorModule(NetCmd *cmd)
{
	switch(cmd->code){
	case READ_VERSION:
		if(UartSend(fd_RS485_index_2, PackByte(&cmd->pvar[0]), MOTOR_COM_READ_VERSION, 0) != 0)
		{
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_Motor_Err[PackByte(&cmd->pvar[0]) - 1].Communication_Err));
		}else
		{
			char version[4][9] = {""};
			uint8_t len = PackByte(_ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[0])].point);
			len += 2;
			int i = 0;
			int n = (len / 8) + (len % 8 == 0 ? 0 : 1);
			for(i = 0; i < n; i++)
			{
				if(i == n - 1)
				{
					if((len % 8) == 0)
					{
						memcpy(version[i],  _ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[0])].point + (8 * i), 8);
					}else
					{
						memcpy(version[i],  _ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[0])].point + (8 * i), len % 8);
						memset(version[i] + (len % 8), '0', 8 - (len % 8));
					}

				}else
				{
					memcpy(version[i],  _ControlBoard[fd_RS485_index_2].Motor[PackByte(&cmd->pvar[0])].point + (8 * i), 8);
				}
			}

			Eth_Send_Queue(cmd, 0, 0xFF, n + 1, 0x0000, version[0], version[1], version[2], version[3]);
		}
		break;
	case SET_MAINIPULATOR_PLACE://设置绝对坐标
		ManipulatorModule_Set_Place(cmd);
		break;
	case MOTOR_RESET://复位
		ManipulatorModule_ResetMotor(cmd);
		break;
	case MOTOR_FREE://机械手_锁定/释放电机
		ManipulatorModule_LockMotor(cmd);
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
			memcpy(My_ClawsPos, point, sizeof(ClawsPos));
			SavePointConfigure(Filename_config, cmd);
			char send_buf0x07[16] = "";
			strncat(send_buf0x07, (char*)&cmd->pvar[2], 4);
			Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, send_buf0x07);
		}else
		{
			//读参数
			char s[1024] = "";
			ReadPointConfigure("POINT", "02", Filename_config, s);
			Eth_Send_Queue(cmd, 0, 0xFF, 2, 0x0000, s);
		}
		break;
	case READ_MAINIPULATOR_PLACE://读机械手位置
		ManipulatorModule_ReadSteps(cmd);
		break;
	case READ_SET_HAND_PLACE://电爪读写位置
		ManipulatorModule_Hand_Read_Set_Place(cmd);
		break;
	case MOVE_TO_TARGET_AREA://移动到指定区域
		ManipulatorModule_Move_To_Target_Aera(cmd);
		break;
	case READ_SAMPLESHELF_BARCODE://读样品架条码
		ManipulatorModule_Read_SampleShelf_BarCode(cmd);
		break;
	case OpenOrCloseScan:////打开/关闭扫码枪
		if(ManipulatorModule_OpenOrCloseScan(cmd, 0) != 0)
		{
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(ManipulatorModuleCommandAdd, ManipulatorModule_OPENBARCODE_Err));
		}
		break;
	case MOVE_TEST_TUBE://移动试管
		ManipulatorModule_MoveTestTube(cmd);
		break;
	case READ_WRITE_MOTOR_PARA://读写电机参数
		ManipulatorModule_MotorPara(cmd);
		break;
	}
}
