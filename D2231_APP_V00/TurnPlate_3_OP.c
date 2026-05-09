/*
 * TurnPlate_3_OP.c
 *
 *  Created on: 2026-1-6
 *      Author: Administrator
 */

#include "TurnPlate_3_OP.h"

int Turntable_3_trunNum = 0;
int Turntable_3_lastLocatin = 1;
char Turntable_3_coord[9] = {0};

//光耦6 无RFID
int Sensor_6_inhand = 0;
int Sensor_5_inhand = 0;
void Turntable_3_Sensor_6(int val)
{
	static struct timeval Time_start;
	static struct timeval Time_now;
	char result;
	if(Sensor_6_inhand == 0 && Sensor_5_inhand == 0 && _State_Moudle.State_TurnPlate_3_Module == State_NoBusy && Get_Trak_State() == 1 && GetTurntableRespond() == 1)
	{
		if(BIT(val, 5) == 0)//表示传感器6触发过
		{
			if(Turntable_3_lastLocatin == 1)
				Turntable_3_trunNum--;
			snprintf(Turntable_3_coord, sizeof(Turntable_3_coord), "%08X",Turntable_3_trunNum * 20480 + railTurntablePos -> portPos[3]);
			Sensor_6_inhand = STEP_1;
		}
	}
	if(Sensor_6_inhand == STEP_1)
	{
		if((result = TurnPlate_3_Move(Turntable_3_coord)) == 0)
		{
			gettimeofday(&Time_start, NULL);
			Sensor_6_inhand = STEP_2;
			_State_Moudle.State_TurnPlate_3_Module = State_Busy;
		}
		else if(result == 1)
		{
			packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_Communication_Err);
			return;
		}
		else if(result == 2)
		{
			packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_Collision);
			return;
		}
	}
	if(Sensor_6_inhand == STEP_2)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, railTurntablePos -> waitingTime[3]) == 0)
		{
			snprintf(Turntable_3_coord, sizeof(Turntable_3_coord), "%08X",Turntable_3_trunNum * 20480 + railTurntablePos -> portPos[2]);
			Sensor_6_inhand = STEP_3;
			_State_Moudle.State_TurnPlate_3_Module = State_NoBusy;
		}
	}
	if(Sensor_6_inhand == STEP_3 && Get_Trak_State() == 1)
	{
		if((result = TurnPlate_3_Move(Turntable_3_coord)) == 0)
		{
			gettimeofday(&Time_start, NULL);
			Sensor_6_inhand = STEP_4;
			_State_Moudle.State_TurnPlate_3_Module = State_Busy;
		}
		else if(result == 1)
		{
			packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_Communication_Err);
			return;
		}
		else if(result == 2)
		{
			packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_Collision);
			return;
		}
	}
	if(Sensor_6_inhand == STEP_4)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, railTurntablePos -> waitingTime[2]) == 0)
		{
			Turntable_3_lastLocatin = 3;
			Sensor_6_inhand = 0;
			_State_Moudle.State_TurnPlate_3_Module = State_NoBusy;
		}
	}
}

char turntable_3_release;
void Turntable_3_Sensor_5(int val)
{
	static struct timeval Time_start;
	static struct timeval Time_now;
	char result;

	message _message;
	if(Sensor_5_inhand == 0 && Sensor_6_inhand == 0 && _State_Moudle.State_TurnPlate_3_Module == State_NoBusy && Get_Trak_State() == 1 && GetTurntableRespond() == 1)
	{
		if(BIT(val, 4) == 0)//表示传感器5触发过
		{
			if(BIT(val, 5) == 1)//传感器6优先
			{
				Sensor_5_inhand = STEP_1;
				gettimeofday(&Time_start, NULL);
			}
		}
	}
	if(Sensor_5_inhand == STEP_1)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, RFID_MAX_WAIT_TIME_3) == 0)
		{
			if(Get_Device_Mode() == 1 || Get_Device_Mode() == 3)
			{
				turntable_3_release = 1;
			}
			else if(Get_Device_Mode() == 5)
			{
				turntable_3_release = 1;
				if((result = Read_RFID(fd_RS485_index_3, RFID_4_ADDR, TRACK_OFFSET_OFFSET)) != 0x08)
				{
					if(result == 1 || result == 2)
					{
						packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_RFID_FindCard_Err);
					}
					else if(result == 3)
					{
						packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_RFID_Communication_Err);
					}
				}
			}
			else if(Get_Device_Mode() == 2 || Get_Device_Mode() == 6)
			{
				if((result = Read_RFID_Barcode2(fd_RS485_index_3, RFID_4_ADDR, &_message)) == 0)
				{
					if(_message.BarCode_len == 0)
					{
						turntable_3_release = 1;
					}
					else
					{
						char isNeedCentrifuge = _ControlBoard[fd_RS485_index_3].Motor[RFID_4_ADDR].point[23 + _message.BarCode_len];
						char isHasCentrifuge = _ControlBoard[fd_RS485_index_3].Motor[RFID_4_ADDR].point[24 + _message.BarCode_len];
						turntable_3_release = IsTurnplateRelease();
						if(turntable_3_release == 3 || turntable_3_release == 4)
						{
							packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_RFID_FindCard_Err);
							return;
						}
						else if(turntable_3_release == 5)
						{
							packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_RFID_Communication_Err);
							return;
						}
						else if(turntable_3_release == 1)
						{
							writeTraInfo(fd_RS485_index_3, RFID_4_ADDR, &_message, 18, NULL);
						}
						else
						{
							char record[5];
							writeTraInfo(fd_RS485_index_3, RFID_4_ADDR, &_message, 18, record);
							char trajData[200];
							memset(trajData, 0, 200);
							trajData[0] = '>';
							sprintf(&trajData[1], "%08X", 0xFFFFFFFE);
							sprintf(&trajData[9], "04020000000");
							sprintf(&trajData[20], "%08X", _message.CAR_ID);
							sprintf(&trajData[28], "%02X", _message.BarCode_len);
							memcpy(&trajData[30], _message.BarCode, _message.BarCode_len);

							if((result = Read_RFID(fd_RS485_index_3, RFID_4_ADDR, _message._04_offset)) == 0x08)
							{
								memcpy(&trajData[30 + _message.BarCode_len], _ControlBoard[fd_RS485_index_3].Motor[RFID_4_ADDR].point + 17, 89);
								memcpy(&trajData[107 + _message.BarCode_len], record, 4);
								trajData[119 + _message.BarCode_len] = '0';
								trajData[120 + _message.BarCode_len] = isNeedCentrifuge;
								trajData[121 + _message.BarCode_len] = isHasCentrifuge;
								trajData[122 + _message.BarCode_len] = '\0';
								char send_crcbuf[5];
								crc_check((uint8_t *)trajData, send_crcbuf);
								strcat(trajData, send_crcbuf);
								strcat(trajData, "\r\n");
								enQueue(PQueue_ETH_Send, trajData, strlen(trajData));
							}
							else if(result == 1 || result == 2)
							{
								packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_RFID_FindCard_Err);
								return;
							}
							else if(result == 3)
							{
								packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_RFID_Communication_Err);
								return;
							}
						}
					}
				}
				else if(result == 1)
				{
					turntable_3_release = 1;
				}
				else if(result == 3 || result == 4)
				{
					packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_RFID_FindCard_Err);
					return;
				}
				else if(result == 5)
				{
					packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_RFID_Communication_Err);
					return;
				}
			}
			else if(Get_Device_Mode() == 4)
			{
				turntable_3_release = 0;
			}
			snprintf(Turntable_3_coord, sizeof(Turntable_3_coord), "%08X",Turntable_3_trunNum * 20480 + railTurntablePos -> portPos[1]);
			Sensor_5_inhand = STEP_2;
		}
	}
	if(Sensor_5_inhand == STEP_2)
	{
		if((result = TurnPlate_3_Move(Turntable_3_coord)) == 0)
		{
			gettimeofday(&Time_start, NULL);
			Sensor_5_inhand = STEP_3;
			_State_Moudle.State_TurnPlate_3_Module = State_Busy;
		}
		else if(result == 1)
		{
			packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_Communication_Err);
			return;
		}
		else if(result == 2)
		{
			packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_Collision);
			return;
		}
	}
	if(Sensor_5_inhand == STEP_3)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, railTurntablePos -> waitingTime[1]) == 0)
		{
			if(turntable_3_release == 1)
			{
				snprintf(Turntable_3_coord, sizeof(Turntable_3_coord), "%08X",Turntable_3_trunNum * 20480 + railTurntablePos -> portPos[0]);
			}
			else
			{
				snprintf(Turntable_3_coord, sizeof(Turntable_3_coord), "%08X",Turntable_3_trunNum * 20480 + railTurntablePos -> portPos[2]);
			}
			Sensor_5_inhand = STEP_4;
			_State_Moudle.State_TurnPlate_3_Module = State_NoBusy;
		}
	}
	if(Sensor_5_inhand == STEP_4 && Get_Trak_State() == 1)
	{
		if((result = TurnPlate_3_Move(Turntable_3_coord)) == 0)
		{
			gettimeofday(&Time_start, NULL);
			Sensor_5_inhand = STEP_5;
			_State_Moudle.State_TurnPlate_3_Module = State_Busy;
		}
		else if(result == 1)
		{
			packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_Communication_Err);
			return;
		}
		else if(result == 2)
		{
			packageErr(TurnPlate_3_ModuleCommandAdd, TurnPlate_3_Module_Collision);
			return;
		}
	}
	if(Sensor_5_inhand == STEP_5)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, turntable_3_release == 1 ? railTurntablePos -> waitingTime[0] : railTurntablePos -> waitingTime[2]) == 0)
		{
			Turntable_3_lastLocatin = (turntable_3_release == 1 ? 1 : 3);
			Sensor_5_inhand = 0;
			_State_Moudle.State_TurnPlate_3_Module = State_NoBusy;
		}
	}
}

/**
 * 测试模式
 *
 * 0 - 不放行
 * 		测试未完成
 * 1 - 放行
 * 		空小车
 * 		测试异常
 * 		测试完成
 * 3，4 - 寻卡失败
 * 5 - 通信失败
 */
char IsTurnplateRelease()
{
	message _message;
	int dataLen = 0;
	char data[1000];
	char err;
	if((err = Read_RFID(fd_RS485_index_3, RFID_4_ADDR, TRACK_OFFSET_OFFSET)) == 0x08)//读轨迹信息的偏移扇区，固定从偏移0x04扇区读
	{
		if(PackByte(_ControlBoard[fd_RS485_index_3].Motor[RFID_4_ADDR].point + 12) == 0x7E)
		{
			_message._04_hand = 0x7E;
			_message._04_offset = PackByte(_ControlBoard[fd_RS485_index_3].Motor[RFID_4_ADDR].point + 14) * 4;
			if((err = Read_RFID(fd_RS485_index_3, RFID_4_ADDR, _message._04_offset + 4)) == 0x08)//读小车存储的条码
			{
				_message.BarCode_len = PackByte(_ControlBoard[fd_RS485_index_3].Motor[RFID_4_ADDR].point + 21);
				if(_message.BarCode_len == 0)
				{
					return 1;
				}
				else
				{
					dataLen = PackWord(_ControlBoard[fd_RS485_index_3].Motor[RFID_4_ADDR].point + 17) + 4;
					int sectorNum = dataLen / 91 + (dataLen % 91 == 0 ? 0 : 1);
					memcpy(data, _ControlBoard[fd_RS485_index_3].Motor[RFID_4_ADDR].point + 17, 91);
					if(dataLen < _message.BarCode_len + 10 && data[_message.BarCode_len + 6] == '1')
					{
						//需要离心，还没有写入测试任务
						return 0;
					}
					int testState;
					//未测试
					int notTest = 1;
					//有异常
					int testexcep = 1;
					int k;
					for(k = 0;k < 4;k++)
					{
						if(Get_Branch_State(k) == 1)
						{
							testState = PackByte(&data[_message.BarCode_len + 12 + k * 2]);
							if(testState != 0)
							{
								notTest = 0;
							}
							if(testState <= 1 || testState == 5 || testState == 6)
							{
								testexcep = 0;
							}
						}
					}

					//未测试
					if(notTest == 1)
					{
						return 0;
					}
					//测试异常
					if(testexcep == 1)
					{
						return 1;
					}
					int i,j;
					for(i = 1;i < sectorNum;i++)
					{
						if((err = Read_RFID(fd_RS485_index_3, RFID_4_ADDR, _message._04_offset + 4 + i * 4)) == 0x08)//读小车存储的条码
						{
							for(j = 0;j < 91;j++)
							{
								data[91 * i + j] = _ControlBoard[fd_RS485_index_3].Motor[RFID_4_ADDR].point[j + 17];
							}
						}
						else
						{
							return err + 2;
						}
					}
					int testNum = PackByte(&data[_message.BarCode_len + 20]);
					for(i = 0;i < testNum;i++)
					{
						if(AsciiToHex(data[_message.BarCode_len + 22 + 9 * i]) == 0)
						{
							//测试未完成
							return 0;
						}
					}
					//测试完成
					return 1;
				}
			}
			else
			{
				return err + 2;
			}
		}
		else
		{
			//未获取到读轨迹信息的偏移扇区
			char buf[128] = {0};
			int track_offset = 0x08;
			sprintf(buf, "7E%02X", track_offset);
			if((err = Write_RFID(fd_RS485_index_3, RFID_4_ADDR, TRACK_OFFSET_OFFSET, buf)) == 0x08)
			{
				return 1;
			}
			else
			{
				return err + 2;
			}
		}
	}
	else
	{
		return err + 2;
	}
}
