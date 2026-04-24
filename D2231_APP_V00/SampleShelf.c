/*
 * SampleShelf.c
 *
 *样本架模块
 *
 *  Created on: 2025-12-31
 *      Author: ThinkPad
 */
#include "SampleShelf.h"

#define Read_SampleShelf_State_Timeout   50//ms


#define SAMPLESHELF_READSTATE			 0x11//读状态
#define LED_CONTROL						 0x13//LED控制
#define BUZZE_CONTROL					 0x17//蜂鸣器控制


void SampleShelf_ReadState(NetCmd *cmd)
{
	//发送>082读状态，间隔50ms
	static struct timeval Time_start;
	static struct timeval Time_now;
	gettimeofday(&Time_now, NULL);
	if(My_timeout(&Time_start, &Time_now, Read_SampleShelf_State_Timeout) == 0)
	{
		if(UartSend(fd_RS485_index_0, SampleShelf_Mode_Add, MAGENT_COM_READSTEPS, 0) != 0)
		{
			//通讯失败，报错
		}else
		{
			//通讯成功，解析状态
			_MagentBoard_STATE_tmp.Microswitch_state = PackWord(_ControlBoard[fd_RS485_index_0].Motor[SampleShelf_Mode_Add].point + 4);
			_MagentBoard_STATE_tmp.LED_state = Pack8Byte(_ControlBoard[fd_RS485_index_0].Motor[SampleShelf_Mode_Add].point + 5)  & 0x000FFFFF;
			_MagentBoard_STATE_tmp.KEY_state = PackWord(_ControlBoard[fd_RS485_index_0].Motor[SampleShelf_Mode_Add].point + 13);
			int i = 0;
			int n = 0;
			char _static[16] = "";
			i = calc_diff_bit(_MagentBoard_STATE.Microswitch_state, _MagentBoard_STATE_tmp.Microswitch_state);
			if(i > 0)
			{
				//上反
				for(; n < 10; n++)
				{
					if(BIT(i, n) != 0)
					{
						sprintf(_static, "2%01X%01X00000", n + 1, getBit(_MagentBoard_STATE_tmp.Microswitch_state, n ));
						Eth_Send_Queue(cmd, 0, 0xFF, 2, 0000, _static);
					}
				}
				_MagentBoard_STATE.Microswitch_state = _MagentBoard_STATE_tmp.Microswitch_state;
			}
			i = calc_diff_bit(_MagentBoard_STATE.LED_state, _MagentBoard_STATE_tmp.LED_state);
			if(_MagentBoard_STATE.LED_state != _MagentBoard_STATE_tmp.LED_state)
			{
				//上反
				for(n = 0; n < 20;)
				{
					if(((_MagentBoard_STATE.LED_state >> n) & 0x3) != ((_MagentBoard_STATE_tmp.LED_state >> n) & 0x3))
					{
						sprintf(_static, "3%01X%01X00000", (n / 2) + 1, ((_MagentBoard_STATE_tmp.LED_state >> n) & 0x3));
						Eth_Send_Queue(cmd, 0, 0xFF, 2, 0000, _static);
					}
					n = n + 2;
				}
				_MagentBoard_STATE.LED_state = _MagentBoard_STATE_tmp.LED_state;
			}
			i = calc_diff_bit(_MagentBoard_STATE.KEY_state, _MagentBoard_STATE_tmp.KEY_state);
			if(i > 0)
			{
				//上反
				for(n = 0; n < 10; n++)
				{
					if(BIT(i, n) != 0)//有变化
					{
						if(BIT(_MagentBoard_STATE_tmp.KEY_state, n) != 0)//按键状态从0到1（从1到0不触发上返）
						{
							sprintf(_static, "1%01X%01X00000", n + 1, getBit(_MagentBoard_STATE_tmp.KEY_state, n ));
							Eth_Send_Queue(cmd, 0, 0xFF, 2, 0000, _static);
						}
					}
				}
				_MagentBoard_STATE.KEY_state = _MagentBoard_STATE_tmp.KEY_state;
			}
		}
		gettimeofday(&Time_start, NULL);
	}
}

void SampleShelf_Led_Control(NetCmd *cmd)
{
	uint32_t LED_set_state = 0;
	char LED_set_state_buf[6] = {0};
	int i = 0;
	for(i = 0; i < 10;i ++)
	{
		LED_set_state += (AsciiToHex(cmd->pvar[i]) << (i * 2));
	}
	sprintf(LED_set_state_buf, "%05X", LED_set_state);
	if(UartSend(fd_RS485_index_0, SampleShelf_Mode_Add, MAGENT_COM_LED_CONTROL, 1, LED_set_state_buf) != 0)
	{
		//通讯失败，报错
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(SampleShelf_ModuleCommandAdd, SampleShelfModule_Communication_Err));
	}else
	{
		Eth_Send_Queue(cmd, 0, 0xFF, 1, 0000);
	}
}

/*
 * 开蜂鸣器
 */
void BuzzerOpen(NetCmd *cmd)
{
	unsigned int Datanum[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	Datanum[0] = AsciiToHex(cmd->pvar[0]);
	Datanum[1] = PackByte((char *)&cmd->pvar[1]);
	if(CanSendCommand(1, 0, OpenBuzzer_Command,Datanum) != 1)//发送不成功
	{
		_ControlBoard[5].Motor[0].State[OpenBuzzer_Command] = State_Error;
		Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(SampleShelf_ModuleCommandAdd, SampleShelfModule_Buzzer_Communication_Err));
	}else
	{
		Eth_Send_Queue(cmd, 0, 0xFF, 1, 0x00);
	}
}


void SampleShelfModule(NetCmd *cmd)
{
	switch(cmd->code){
	case READ_VERSION:
		if(UartSend(fd_RS485_index_0, SampleShelf_Mode_Add, MAGENT_COM_READ_VERSION, 0) != 0)
		{
			Eth_Send_Queue(cmd, 0, 0xFF, 1, CompoundErrorCode(SampleShelf_ModuleCommandAdd, SampleShelfModule_Communication_Err));
		}else
		{
			int len = PackByte(_ControlBoard[fd_RS485_index_0].Motor[SampleShelf_Mode_Add].point);
			len += 2;
			char version[4][9] = {""};
			int i = 0;
			int n = (len / 8) + (len % 8 == 0 ? 0 : 1);
			for(i = 0; i < n; i++)
			{
				if(i == n - 1)
				{
					memcpy(version[i],  _ControlBoard[fd_RS485_index_0].Motor[SampleShelf_Mode_Add].point + (8 * i), len % 8);
					memset(version[i] + (len % 8), '0', 8 - (len % 8));
				}else
				{
					memcpy(version[i],  _ControlBoard[fd_RS485_index_0].Motor[SampleShelf_Mode_Add].point + (8 * i), 8);
				}
			}

			Eth_Send_Queue(cmd, 0, 0xFF, n + 1, 0x0000, version[0], version[1], version[2], version[3]);
		}
		break;
	case SAMPLESHELF_READSTATE://读状态
	{
		char _state_1[9] = "";
		char _state_2[9] = "";
		char _state_3[9] = "";
		int n  = 0;
		for(n = 0; n < 8; n++)
		{
			sprintf(_state_1 + n, "%01X", getBit(_MagentBoard_STATE_tmp.Microswitch_state, n ));
		}
		for(n = 0; n < 2; n++)
		{
			sprintf(_state_2 + n, "%01X", getBit(_MagentBoard_STATE_tmp.Microswitch_state, (n + 8)));
		}
		for(n = 2; n < 8; n++)
		{
			sprintf(_state_2 + n, "%01X", ((_MagentBoard_STATE_tmp.LED_state >> ((n * 2) - 4)) & 0x3));
		}
		for(n = 4; n < 8; n++)
		{
			sprintf(_state_3 + (n - 4), "%01X", ((_MagentBoard_STATE_tmp.LED_state >> ((n * 2) + 4)) & 0x3));
		}
		sprintf(_state_3, "%s0000", _state_3);
		Eth_Send_Queue(cmd, 0, 0xFF, 4, 0000, _state_1, _state_2, _state_3);
	}
		break;
	case LED_CONTROL:
		SampleShelf_Led_Control(cmd);
		break;
	case BUZZE_CONTROL:
		BuzzerOpen(cmd);
		break;

	}
}
