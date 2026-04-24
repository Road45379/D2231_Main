/*
 * RFID.c
 *
 *  Created on: 2026-1-4
 *      Author: ThinkPad
 */
#include "RFID.h"
#include "Set_Time.h"

#define TRACK_OFFSET_OFFSET 0x04
int track_offset = 0;

int Read_RFID(int UART_ADDR, int RFID_ADDR, int offset)
{
	char offset_buf[3] = {0};
	sprintf(offset_buf, "%02X", offset);
	if(UartSend(UART_ADDR, RFID_ADDR, READ_RFID, 1, offset_buf) != 0)
	{
		//有通讯故障，报错
		return 3;
	}else
	{
		return _ControlBoard[UART_ADDR].Motor[RFID_ADDR].State[READ_RFID - 'A'];
	}
}

int Write_RFID(int UART_ADDR, int RFID_ADDR, int offset, char * buf)
{
	char offset_buf[3] = {0};
	sprintf(offset_buf, "%02X", offset);
	if(UartSend(UART_ADDR, RFID_ADDR, WRITE_RFID, 2, offset_buf, buf) != 0)
	{
		//有通讯故障，报错
		return 3;
	}else
	{
		return _ControlBoard[UART_ADDR].Motor[RFID_ADDR].State[READ_RFID - 'A'];
	}
}

int clear_car_message()
{
	int i = 0;
	char buf[97] = {0};
	for(i = 0; i < 96; i++)
	{
		buf[i] = '0';
	}
	int err = 0;
	for(i = 1; i < 16; i++)
	{
		err += Write_RFID(fd_RS485_index_1, RFID_1_ADDR, i * 4, buf);
	}
	return err;
}

/*
 * 清除小测信息
 */
int clear_car_tarck_or_message(int RFID_ADDR, uint8_t flag, uint32_t car_id)
{
	if(flag == 0)//清除小车所有信息，相当于恢复出厂设置
	{
		return clear_car_message();
	}else
	{
		if(Read_RFID(fd_RS485_index_1,RFID_ADDR, TRACK_OFFSET_OFFSET) == 0x08)//读轨迹信息的偏移扇区，固定从偏移0x04块读
		{
			int track_write_num = 0;//轨迹信息写入次数
			char buf_message[128] = {0};
			_message_TurnPlate_1_1._04_hand =  0x7E;
			_message_TurnPlate_1_1._04_offset = (PackByte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 14) * 4);
			_message_TurnPlate_1_1.CAR_ID = Pack8Byte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 2);
			if(_message_TurnPlate_1_1.CAR_ID == car_id)//小车ID校验通过
			{
				if(BIT(flag, 0) == 1)//清轨迹信息
				{
					if(Read_RFID(fd_RS485_index_1,RFID_ADDR, _message_TurnPlate_1_1._04_offset) == 0x08)//读小车轨迹信息
					{
						_message_TurnPlate_1_1.Wirte_Num = PackWord(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 13);
						_message_TurnPlate_1_1.Wirte_Num += (AsciiToHex(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point[12]) << 16);//写入次数
						_message_TurnPlate_1_1.Wirte_Num++;

						track_write_num = _message_TurnPlate_1_1.Wirte_Num;

						char buf_tarck[128] = {0};
						sprintf(buf_tarck, "%05X", _message_TurnPlate_1_1.Wirte_Num);
						memset(buf_tarck + 5, '0', 91);
						Write_RFID(fd_RS485_index_1, RFID_ADDR, _message_TurnPlate_1_1._04_offset, buf_tarck);//轨迹信息写0

					}else
					{
						return -1;//轨迹信息清失败
					}
				}
				if(BIT(flag, 1) == 1)//清测试信息
				{
					if(Read_RFID(fd_RS485_index_1,RFID_ADDR, _message_TurnPlate_1_1._04_offset + 0x04) == 0x08)//读小车存储的条码，获取长度
					{
						_message_TurnPlate_1_1.Wirte_Num = PackWord(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 13);
						_message_TurnPlate_1_1.Wirte_Num += (AsciiToHex(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point[12]) << 16);//写入次数
						_message_TurnPlate_1_1.Wirte_Num++;

						_message_TurnPlate_1_1.message_len = PackWord(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 17);

						if((_message_TurnPlate_1_1.message_len >= 0) && (_message_TurnPlate_1_1.message_len <= 178))//有测试信息
						{
							//清第一个扇区
							sprintf(buf_message, "%05X", _message_TurnPlate_1_1.Wirte_Num);
							memset(buf_message + 5, '0', 91);
							Write_RFID(fd_RS485_index_1, RFID_ADDR, _message_TurnPlate_1_1._04_offset  + 0x04, buf_message);//测试信息写0
						}else
						{
							return -3;//数据长度异常
						}
						if((_message_TurnPlate_1_1.message_len > 87) && (_message_TurnPlate_1_1.message_len <= 178))
						{
							//清第二个扇区
							if(Read_RFID(fd_RS485_index_1,RFID_ADDR, _message_TurnPlate_1_1._04_offset + 0x08) == 0x08)//读小车存储的条码，获取写入次数
							{
								_message_TurnPlate_1_1.Wirte_Num = PackWord(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 13);
								_message_TurnPlate_1_1.Wirte_Num += (AsciiToHex(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point[12]) << 16);//写入次数
								_message_TurnPlate_1_1.Wirte_Num++;


								sprintf(buf_message, "%05X", _message_TurnPlate_1_1.Wirte_Num);
								memset(buf_message + 5, '0', 91);
								Write_RFID(fd_RS485_index_1, RFID_ADDR, _message_TurnPlate_1_1._04_offset  + 0x08, buf_message);//测试信息写0

							}
						}
					}else
					{
						return -2;//测试信息清失败
					}
				}

				if(track_write_num >= 95000)//轨迹信息写入大于95000次，后移一个扇区
				{
					sprintf(buf_message, "7E%02X", _message_TurnPlate_1_1._04_offset + 1);
					memset(buf_message + 4, '0', 92);
					Write_RFID(fd_RS485_index_1, RFID_ADDR, TRACK_OFFSET_OFFSET, buf_message);
				}
			}else
			{
				return -4;//小车ID校验失败
			}
		}

	}
	return 0;
}

/*
 * reutrn :
 * 1 : 寻卡失败
 * 0 ： 成功
 * 3 : 通讯失败
 */
int Read_RFID_Barcode(int RFID_ADDR)//读小车存储的条码
{
	int err = Read_RFID(fd_RS485_index_1,RFID_ADDR, TRACK_OFFSET_OFFSET);
	if(err == 0x08)//读轨迹信息的偏移扇区，固定从偏移0x04块读
	{
		if(PackByte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 12) == 0x7E)
		{
			_message_TurnPlate_1_1._04_hand =  0x7E;
			_message_TurnPlate_1_1._04_offset = (PackByte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 14) * 4);
			_message_TurnPlate_1_1.CAR_ID = Pack8Byte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 2);

			err = Read_RFID(fd_RS485_index_1,RFID_ADDR, _message_TurnPlate_1_1._04_offset);
			if(err == 0x08)//读小车轨迹信息
			{
				memcpy(_message_TurnPlate_1_1.tarck, _ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 17,  89);
			}else
			{
				memset(_message_TurnPlate_1_1.tarck, '0',  96);
				if(err == 0x01 || err == 0x02)//寻卡失败
				{
					return 1;
				}else//通讯失败
				{
					return 3;
				}
			}
			err = Read_RFID(fd_RS485_index_1,RFID_ADDR, _message_TurnPlate_1_1._04_offset + 0x04);
			if(err == 0x08)//读小车存储的条码
			{
				_message_TurnPlate_1_1.BarCode_len = PackByte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 21);
				_message_TurnPlate_1_1.message_len = PackWord(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 17);
				if(_message_TurnPlate_1_1.BarCode_len > 0)
				{
					memset(_message_TurnPlate_1_1.BarCode, 0, 16);
					memcpy(_message_TurnPlate_1_1.BarCode, _ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 23,  _message_TurnPlate_1_1.BarCode_len);
				}
				if(_message_TurnPlate_1_1.message_len > 0)
				{
					memset(_message_TurnPlate_1_1.message, 0, 192);
					int sectorNum = (_message_TurnPlate_1_1.message_len + 90) / 91;
					if(sectorNum > 1 && sectorNum < 3)
					{
						int message_offset = 96 - 11 - _message_TurnPlate_1_1.BarCode_len;
						if(message_offset >= 0)
						{
							memcpy(_message_TurnPlate_1_1.message, _ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 23 + _message_TurnPlate_1_1.BarCode_len,  message_offset);
						}
						int i = 0;
						for(i = 1; i < sectorNum; i++)
						{
							err = Read_RFID(fd_RS485_index_1,RFID_ADDR, _message_TurnPlate_1_1._04_offset + ((i + 1) * 0x04));
							if(err == 0x08)//读小车存储的测试信息
							{
								if(i == (sectorNum - 1))
								{
									memcpy(_message_TurnPlate_1_1.message + message_offset, _ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 17, (_message_TurnPlate_1_1.message_len + 4) % 91);
								}else
								{
									memcpy(_message_TurnPlate_1_1.message + message_offset, _ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 17, 91);
									message_offset += 91;
								}
							}else if(err == 0x01 || err == 0x02)//寻卡失败
							{
								return 1;
							}else//通讯失败
							{
								return 3;
							}
						}
					}else if(sectorNum == 1)
					{
						memcpy(_message_TurnPlate_1_1.message, _ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 23 + _message_TurnPlate_1_1.BarCode_len,  _message_TurnPlate_1_1.message_len - 2 - _message_TurnPlate_1_1.BarCode_len);
					}
				}
				return 0;
			}else
			{
				_message_TurnPlate_1_1.BarCode_len = 0;
				memset(_message_TurnPlate_1_1.BarCode, 0,  16);
				if(err == 0x01 || err == 0x02)//寻卡失败
				{
					return 1;
				}else//通讯失败
				{
					return 3;
				}

			}
		}else
		{
			//未获取到读轨迹信息的偏移扇区
			_message_TurnPlate_1_1.CAR_ID = Pack8Byte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 2);
			char buf[128] = {0};
			track_offset = 0x02;
			sprintf(buf, "7E%02X", track_offset);
			err = Write_RFID(fd_RS485_index_1, RFID_ADDR, TRACK_OFFSET_OFFSET, buf);
			if(err == 0x08)
			{
				return 0;
			}else if(err == 0x01 || err == 0x02)//寻卡失败
			{
				return 1;
			}else if(err == 0x03)//通讯失败
			{
				return 3;
			}else
			{
				return 0;
			}
		}
	}else if(err == 0x01 || err == 0x02)//寻卡失败
	{
		return 1;
	}else//通讯失败
	{
		return 3;
	}
}

/*
 * reutrn :
 * 1 : 寻卡失败
 * 0 ： 成功
 * 3 : 通讯失败
 */
int Write_RFID_Barcode(int RFID_ADDR, char *BarCode, int len)//写小车存储的条码,轨迹（起始时间）
{
	int err = Read_RFID(fd_RS485_index_1,RFID_ADDR, TRACK_OFFSET_OFFSET);
	if(err == 0x08)//读轨迹信息的偏移扇区，固定从偏移0x04块读
	{
		if(PackByte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 12) == 0x7E)
		{
			_message_TurnPlate_1_1._04_hand =  0x7E;
			_message_TurnPlate_1_1._04_offset = (PackByte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 14) * 4);
			_message_TurnPlate_1_1.CAR_ID = Pack8Byte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 2);
			err = Read_RFID(fd_RS485_index_1,RFID_ADDR, _message_TurnPlate_1_1._04_offset);
			if(err == 0x08)//读小车轨迹信息写入次数
			{
				_message_TurnPlate_1_1.Wirte_Num = PackWord(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 13);
				_message_TurnPlate_1_1.Wirte_Num += (AsciiToHex(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point[12]) << 16);//写入次数
				_message_TurnPlate_1_1.Wirte_Num++;

				struct tm * stime = ReadSysTime_returnTm();
				tmTodatetime_t(stime, &_dt);
				uint64_t time_uint64_t = datetime_pack_bits(&_dt);
				char buf_tarck[128] = {0};
				sprintf(buf_tarck, "%05X%01X%08X", _message_TurnPlate_1_1.Wirte_Num, (uint32_t)(time_uint64_t >> 32 & 0xF),(uint32_t)(time_uint64_t  & 0xFFFFFFFF));
				Write_RFID(fd_RS485_index_1, RFID_ADDR, _message_TurnPlate_1_1._04_offset, buf_tarck);

			}else if(err == 0x01 || err == 0x02)//寻卡失败
			{
				return 1;
			}else//通讯失败
			{
				return 3;
			}

			err = Read_RFID(fd_RS485_index_1,RFID_ADDR, _message_TurnPlate_1_1._04_offset + 0x04);
			if(err == 0x08)//读小车存储的条码（写入次数）
			{
				_message_TurnPlate_1_1.Wirte_Num = PackWord(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 13);
				_message_TurnPlate_1_1.Wirte_Num += (AsciiToHex(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point[12]) << 16);//写入次数
				_message_TurnPlate_1_1.Wirte_Num++;
				char buf[128] = {0};
				sprintf(buf, "%05X%04X%02X%s", _message_TurnPlate_1_1.Wirte_Num, len + 2, len, BarCode);
				err = Write_RFID(fd_RS485_index_1, RFID_ADDR, _message_TurnPlate_1_1._04_offset + 0x04, buf);
				if(err == 0x08)
				{
					return 0;
				}else if(err == 0x01 || err == 0x02)//寻卡失败
				{
					return 1;
				}else if(err == 0x03)//通讯失败
				{
					return 3;
				}else
				{
					return 0;
				}
			}else
			{

				_message_TurnPlate_1_1.BarCode_len = 0;
				memset(_message_TurnPlate_1_1.BarCode, 0,  16);
				if(err == 0x01 || err == 0x02)//寻卡失败
				{
					return 1;
				}else//通讯失败
				{
					return 3;
				}
			}
		}else
		{
			//未获取到读轨迹信息的偏移扇区
			_message_TurnPlate_1_1.CAR_ID = Pack8Byte(_ControlBoard[fd_RS485_index_1].Motor[RFID_ADDR].point + 2);
			char buf[128] = {0};
			track_offset = 0x02;
			sprintf(buf, "7E%02X", track_offset);
			Write_RFID(fd_RS485_index_1, RFID_ADDR, TRACK_OFFSET_OFFSET, buf);

			sprintf(buf, "%05X0000%02X%s", 0, len, BarCode);
			err = Write_RFID(fd_RS485_index_1, RFID_ADDR, _message_TurnPlate_1_1._04_offset + 0x04, buf);
			if(err == 0x08)
			{
				return 0;
			}else if(err == 0x01 || err == 0x02)//寻卡失败
			{
				return 1;
			}else if(err == 0x03)//通讯失败
			{
				return 3;
			}else
			{
				return 0;
			}

		}
	}else if(err == 0x01 || err == 0x02)//寻卡失败
	{
		return 1;
	}else//通讯失败
	{
		return 3;
	}

}

//RE 增加
/************
 *  0 - 无错误
 *  1 - 小车第一次使用
 *  2 - 通信失败
 */
int Read_RFID_Barcode2(int UART_ADDR, int RFID_ADDR, message *_message)//读小车存储的条码
{
	if(Read_RFID(UART_ADDR, RFID_ADDR, TRACK_OFFSET_OFFSET) == 0x08)//读轨迹信息的偏移扇区，固定从偏移0x04扇区读
	{
		if(PackByte(_ControlBoard[UART_ADDR].Motor[RFID_ADDR].point + 12) == 0x7E)
		{
			_message ->_04_hand =  0x7E;
			_message ->_04_offset = PackByte(_ControlBoard[UART_ADDR].Motor[RFID_ADDR].point + 14) * 4;
			_message -> CAR_ID  = Pack8Byte(_ControlBoard[UART_ADDR].Motor[RFID_ADDR].point + 2);
			if(Read_RFID(UART_ADDR, RFID_ADDR, _message ->_04_offset + 0x04) == 0x08)//读小车存储的条码
			{
				_message -> Wirte_Num = PackWord(_ControlBoard[UART_ADDR].Motor[RFID_ADDR].point + 13);
				_message -> Wirte_Num += (AsciiToHex(_ControlBoard[UART_ADDR].Motor[RFID_ADDR].point[12]) << 16);//写入次数
				_message -> BarCode_len = PackByte(_ControlBoard[UART_ADDR].Motor[RFID_ADDR].point + 21);
				memcpy(_message -> BarCode, _ControlBoard[UART_ADDR].Motor[RFID_ADDR].point + 23,  _message -> BarCode_len);
				return 0;
			}
			else
			{
				return 2;
			}
		}
		else
		{
			//未获取到读轨迹信息的偏移扇区
			char buf[128] = {0};
			track_offset = 0x02;
			sprintf(buf, "7E%02X", track_offset);
			if(Write_RFID(UART_ADDR, RFID_ADDR, TRACK_OFFSET_OFFSET, buf) == 0x08)
			{
				return 1;
			}
			else
			{
				return 2;
			}
		}
	}
	else
	{
		return 2;
	}
}
