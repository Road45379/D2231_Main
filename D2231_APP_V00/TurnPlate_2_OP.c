/*
 * TurnPlate_2_OP.c
 *
 *  Created on: 2026-1-5
 *      Author: Administrator
 */

#include "TurnPlate_2_OP.h"
#include "Set_Time.h"

int Sensor_3_inhand = 0;
int Sensor_4_inhand = 0;

message _message_turntable2;
int Turntable_2_trunNum = 0;
int Turntable_2_lastLocatin = 1;
char coord[9] = {0};

//光耦3 无RFID
void Turntable_2_Sensor_3(int val)
{
	static struct timeval Time_start;
	static struct timeval Time_now;
	char result;
	if(Sensor_3_inhand == 0 && Sensor_4_inhand == 0 && _State_Moudle.State_TurnPlate_2_Module == State_NoBusy && Get_Trak_State() == 1 && GetTurntableRespond() == 1)
	{
		if(BIT(val, 2) == 0)//表示传感器3触发过
		{
			if(Turntable_2_lastLocatin == 1)
				Turntable_2_trunNum--;
			snprintf(coord, sizeof(coord), "%08X",Turntable_2_trunNum * 20480 + turntable2Pos -> portPos[3]);
			Sensor_3_inhand = STEP_1;
		}
	}
	if(Sensor_3_inhand == STEP_1)
	{
		if((result = TurnPlate_2_Move(coord)) == 0)
		{
			gettimeofday(&Time_start, NULL);
			Sensor_3_inhand = STEP_2;
			_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		}
		else if(result == 1)
		{
			packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_Communication_Err);
			return;
		}
		else if(result == 2)
		{
			packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_Collision);
			return;
		}
	}
	if(Sensor_3_inhand == STEP_2)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, turntable2Pos -> waitingTime[3]) == 0)
		{
			snprintf(coord, sizeof(coord), "%08X",Turntable_2_trunNum * 20480 + turntable2Pos -> portPos[2]);
			Sensor_3_inhand = STEP_3;
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
		}
	}
	if(Sensor_3_inhand == STEP_3 && Get_Trak_State() == 1)
	{
		if((result = TurnPlate_2_Move(coord)) == 0)
		{
			gettimeofday(&Time_start, NULL);
			Sensor_3_inhand = STEP_4;
			_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		}
		else if(result == 1)
		{
			packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_Communication_Err);
			return;
		}
		else if(result == 2)
		{
			packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_Collision);
			return;
		}
	}
	if(Sensor_3_inhand == STEP_4)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, turntable2Pos -> waitingTime[2]) == 0)
		{
			Turntable_2_lastLocatin = 3;
			Sensor_3_inhand = 0;
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
		}
	}
}

//0 - 不放行，1 - 放行，2 - 空小车，条件满足放行
int turntable_2_release_flag = 0;
//0 - 不放行，1 - 放行
char turntable_2_release;

void Turntable_2_Sensor_4(int val)
{
	static struct timeval Time_start;
	static struct timeval Time_now;
	char result;
	if(Sensor_4_inhand == 0 && Sensor_3_inhand == 0 && _State_Moudle.State_TurnPlate_2_Module == State_NoBusy && Get_Trak_State() == 1 && GetTurntableRespond() == 1)
	{
		if(BIT(val, 3) == 0)//表示传感器4触发过
		{
			if(BIT(val, 2) == 1)//传感器3优先
			{
				Sensor_4_inhand = STEP_1;
				gettimeofday(&Time_start, NULL);
			}
		}
	}
	if(Sensor_4_inhand == STEP_1)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, RFID_MAX_WAIT_TIME_2) == 0)
		{
			if(Get_Device_Mode() == 1)
			{
				turntable_2_release_flag = 1;
			}
			else if(Get_Device_Mode() == 5)
			{
				turntable_2_release_flag = 1;
				if((result = Read_RFID(fd_RS485_index_3, RFID_3_ADDR, TRACK_OFFSET_OFFSET)) != 0x08)
				{
					if(result == 1 || result == 2)
					{
						packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_RFID_FindCard_Err);
					}
					else if(result == 3)
					{
						packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_RFID_Communication_Err);
					}
				}
			}
			else if(Get_Device_Mode() == 2 || Get_Device_Mode() == 6)
			{
				if((result = Read_RFID_Barcode2(fd_RS485_index_3, RFID_3_ADDR, &_message_turntable2)) == 0)
				{
					if(_message_turntable2.BarCode_len == 0)
					{
						turntable_2_release_flag = 2;
					}
					else
					{
						writeTraInfo(fd_RS485_index_3, RFID_3_ADDR, &_message_turntable2, 1, NULL);
						turntable_2_release_flag = takeOutWriteData(&list);
						if(turntable_2_release_flag == 3 || turntable_2_release_flag == 4)
						{
							packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_RFID_FindCard_Err);
							return;
						}
						else if(turntable_2_release_flag == 5)
						{
							packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_RFID_Communication_Err);
							return;
						}
					}
				}
				else if(result == 1)
				{
					turntable_2_release_flag = 2;
				}
				else if(result == 3 || result == 4)
				{
					packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_RFID_FindCard_Err);
					return;
				}
				else if(result == 5)
				{
					packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_RFID_Communication_Err);
					return;
				}
			}
			else if(Get_Device_Mode() == 3)
			{
				turntable_2_release_flag = 0;
			}
			else if(Get_Device_Mode() == 4)
			{
				//小车自检模式
				if(Get_Car_Done_Amount() >= Get_Car_Amount())
				{
					turntable_2_release_flag = 0;
				}
				else
				{
					if((result = Read_RFID_Barcode2(fd_RS485_index_3, RFID_3_ADDR, &_message_turntable2)) == 0)
					{
		        		if(Read_RFID(fd_RS485_index_3, RFID_3_ADDR, _message_turntable2._04_offset) == 0x08)//读小车存储的条码
		        		{
		        			int writeTimes = PackWord(_ControlBoard[fd_RS485_index_3].Motor[RFID_3_ADDR].point + 13);
		        			writeTimes += (AsciiToHex(_ControlBoard[fd_RS485_index_3].Motor[RFID_3_ADDR].point[12]) << 16);
		        			writeTimes++;
		        			if(writeTimes > CAR_MAX_WRITE_TIME)
		        			{
		        				turntable_2_release_flag = 1;
		        			}
		        			else
		        			{
		        				turntable_2_release_flag = 0;
		        			}
		        		}
					}
					else if(result == 1)
					{
						turntable_2_release_flag = 0;
					}
					else if(result == 3 || result == 4)
					{
						packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_RFID_FindCard_Err);
						return;
					}
					else if(result == 5)
					{
						packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_RFID_Communication_Err);
						return;
					}
				}
			}

			snprintf(coord, sizeof(coord), "%08X",Turntable_2_trunNum * 20480 + turntable2Pos -> portPos[1]);
			Sensor_4_inhand = STEP_2;
		}
	}
	if(Sensor_4_inhand == STEP_2)
	{
		if((result = TurnPlate_2_Move(coord)) == 0)
		{
			gettimeofday(&Time_start, NULL);
			Sensor_4_inhand = STEP_3;
			_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		}
		else if(result == 1)
		{
			packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_Communication_Err);
			return;
		}
		else if(result == 2)
		{
			packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_Collision);
			return;
		}
	}
	if(Sensor_4_inhand == STEP_3)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, turntable2Pos -> waitingTime[1]) == 0)
		{
			turntable_2_release = (turntable_2_release_flag == 1) || (turntable_2_release_flag == 2 && Get_EmptyCar_IntoTark() == 0);
			if(turntable_2_release == 1)
			{
				snprintf(coord, sizeof(coord), "%08X",Turntable_2_trunNum * 20480 + turntable2Pos -> portPos[0]);
			}
			else
			{
				snprintf(coord, sizeof(coord), "%08X",Turntable_2_trunNum * 20480 + turntable2Pos -> portPos[2]);
			}
			Sensor_4_inhand = STEP_4;
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
		}
	}
	if(Sensor_4_inhand == STEP_4 && Get_Trak_State() == 1)
	{
		if((result = TurnPlate_2_Move(coord)) == 0)
		{
			gettimeofday(&Time_start, NULL);
			Sensor_4_inhand = STEP_5;
			_State_Moudle.State_TurnPlate_2_Module = State_Busy;
		}
		else if(result == 1)
		{
			packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_Communication_Err);
			return;
		}
		else if(result == 2)
		{
			packageErr(TurnPlate_2_ModuleCommandAdd, TurnPlate_2_Module_Collision);
			return;
		}
	}
	if(Sensor_4_inhand == STEP_5)
	{
		gettimeofday(&Time_now, NULL);
		if(My_timeout(&Time_start, &Time_now, turntable_2_release == 1 ? turntable2Pos -> waitingTime[0] : turntable2Pos -> waitingTime[2]) == 0)
		{
			if(Get_Device_Mode() == 4)
			{
				Car_Done_Plus();
				if(Get_Car_Done_Amount() == Get_Car_Amount())
				{
					takeOutAsyncCmd(&asyncCmdlist, 0xFF, 0x0C);
				}
			}
			Turntable_2_lastLocatin = (turntable_2_release == 1 ? 1 : 3);
			Sensor_4_inhand = 0;
			_State_Moudle.State_TurnPlate_2_Module = State_NoBusy;
		}
	}
}

/* datetime_t -> time_t */
static time_t datetime_to_time_t(const datetime_t *dt)
{
    struct tm tm_time;

    tm_time.tm_year = dt->year + 2000 - 1900; // since 1900
    tm_time.tm_mon  = dt->month - 1;          // 0–11
    tm_time.tm_mday = dt->day;
    tm_time.tm_hour = dt->hour;
    tm_time.tm_min  = dt->minute;
    tm_time.tm_sec  = dt->second;
    tm_time.tm_isdst = -1;    // let system decide DST

    return mktime(&tm_time);
}

int64_t datetime_diff_seconds(const datetime_t *dt1,
                              const datetime_t *dt2)
{
    time_t t1 = datetime_to_time_t(dt1);
    time_t t2 = datetime_to_time_t(dt2);

    return (int64_t)difftime(t2, t1);
}

//0 - 成功，1、2 - 寻卡失败，3 - 通信失败
char writeTraInfo(int UART_ADDR, int RFID_ADDR, message *_message, int pos, char *record)
{
	char traInfo[97];
	char err;
	if((err = Read_RFID(UART_ADDR, RFID_ADDR, _message ->_04_offset)) == 0x08)
	{
		memcpy(traInfo, _ControlBoard[UART_ADDR].Motor[RFID_ADDR].point + 12, 96);
		traInfo[96] = '\0';
		//写次数
		int Wirte_Num = PackWord(&traInfo[1]);
		char writeTimesArr[6];
		Wirte_Num += (AsciiToHex(traInfo[0]) << 16);
		Wirte_Num++;
		sprintf(writeTimesArr, "%05X", Wirte_Num);
		memcpy(traInfo, writeTimesArr, 5);

		char startTime[10];
		startTime[9] = '\0';
		//起始时间
		memcpy(startTime, &traInfo[5], 9);
		uint64_t startHex = strtoull(startTime, NULL, 16);
		datetime_t _startTime;
		datetime_unpack_hex(startHex, &_startTime);

		//dt-当前时间
		struct tm *stime = ReadSysTime_returnTm();
		tmTodatetime_t(stime, &_dt);

		//间隔秒数
		int64_t interval = datetime_diff_seconds(&_startTime, &_dt);
		char recordPoint[5];
		int writeStart = 14 + (pos - 1) * 4;
		sprintf(recordPoint, "%04X", (short)interval);
		memcpy(&traInfo[writeStart], recordPoint, 4);
		if(record != NULL)
		{
			memcpy(record, recordPoint, 4);
			return 0;
		}

		if((err = Write_RFID(UART_ADDR, RFID_ADDR, _message -> _04_offset, traInfo)) == 0x08)
		{
			return 0;
		}
		else
		{
			return err;
		}
	}
	else
	{
		return err;
	}
}

//将测试数据放入缓存队列
void putInWriteData(List *list, char *writeData, int dataLen)
{
    Node *pNew = (Node *)malloc(sizeof(Node));
    pNew -> next = NULL;
    int i;
    memset(pNew -> writeData, 0, 1000);
    for(i = 0; i < dataLen; i++)
    {
        pNew -> writeData[i] = writeData[i];
    }
    pNew -> dataLen = dataLen;
    if(list -> head != NULL)
    {
        list -> tail -> next = pNew;
        list -> tail = pNew;
    }
    else
        list -> head = list -> tail = pNew;
}

//取出测试数据并写入IC卡
//0 --- 未找到写数据
//1 --- 找到写数据
//2，3，4 --- RFID报错
char takeOutWriteData(List *list)
{
    Node *pPre = list -> head;
    Node *pTemp = list -> head;
    char isFindData;
    char err;

    int i;
    while(pTemp != NULL)
    {
    	isFindData = _message_turntable2.CAR_ID == Pack8Byte(&pTemp -> writeData[13]);
        if(isFindData == 1)
        {
        	//写测试数据
        	int testDataLen = PackWord((char*)&(pTemp -> writeData[21])) + 4;
        	int sectorNum = testDataLen / 91 + (testDataLen % 91 == 0 ? 0 : 1);
        	char testData[97];
        	for(i = 0;i < sectorNum;i++)
        	{
        		//写次数
        		memset(testData, '0', 96);
        		testData[96] = 0;
        		if(Read_RFID(fd_RS485_index_3, RFID_3_ADDR, _message_turntable2._04_offset + (1 + i) * 4) == 0x08)//读小车存储的条码
        		{
        			int writeTimes = PackWord(_ControlBoard[fd_RS485_index_3].Motor[RFID_3_ADDR].point + 13);
        			writeTimes += (AsciiToHex(_ControlBoard[fd_RS485_index_3].Motor[RFID_3_ADDR].point[12]) << 16);
        			writeTimes++;
        			sprintf(testData, "%05X", writeTimes);
        		}

        		//数据
        		if(i == sectorNum - 1)
        		{
        			if((testDataLen % 91) == 0)
        			{
        				memcpy(&testData[5], &(pTemp -> writeData[i * 91 + 21]), 91);
        			}else
        			{
        				memcpy(&testData[5], &(pTemp -> writeData[i * 91 + 21]), testDataLen % 91);
        			}
        		}
        		else
        		{
        			memcpy(&testData[5], &(pTemp -> writeData[i * 91 + 21]), 91);
        		}
        		if((err = Write_RFID(fd_RS485_index_3, RFID_3_ADDR, _message_turntable2._04_offset + 4 + i * 4, testData)) != 0x08)
        		{
        			return err + 2;
        		}
        	}

        	packageNet(pTemp -> writeData);

        	//删除节点
            if(pTemp == list -> head)
            {
                if(pTemp == list -> tail)
                {
                    list -> head = NULL;
                    list -> tail = NULL;
                }
                else
                {
                    list -> head = list -> head -> next;
                }
            }
            else if(pTemp == list -> tail)
            {
                pPre -> next = NULL;
                list -> tail = pPre;
            }
            else
            {
                pPre -> next = pTemp -> next;
            }

            free(pTemp);
            pTemp = NULL;
        	return 1;
        }

        pPre = pTemp;
        pTemp = pTemp -> next;
    }
    return 0;
}

void InitList(List *list)
{
	list -> head = NULL;
	list -> tail = NULL;
}

void ClearList(List *list)
{
	Node *temp;
	while(list -> head != NULL)
	{
		temp = list -> head;
		list -> head = list -> head -> next;
		free(temp);
	}
	list -> tail = NULL;
}

void packageNet(char *sendBuf)
{
	char send_crcbuf[5];
	sprintf(&sendBuf[11], "%s", "020FF0000");
	crc_check((uint8_t *)sendBuf, send_crcbuf);
	strcat(sendBuf, send_crcbuf);
	strcat(sendBuf, "\r\n");
	enQueue(PQueue_ETH_Send, sendBuf, strlen(sendBuf));
}

void packageErr(char moudle, char err)
{
	char sendBuff[100];
	sendBuff[0] = '>';
	sprintf(&sendBuff[1], "%08X", 0xFFFFFFFC);
	sprintf(&sendBuff[9], "%02X020FF%04X", moudle, CompoundErrorCode(moudle, err));
	char send_crcbuf[5];
	crc_check((uint8_t *)sendBuff, send_crcbuf);
	strcat(sendBuff, send_crcbuf);
	strcat(sendBuff, "\r\n");
	enQueue(PQueue_ETH_Send, sendBuff, strlen(sendBuff));
}
