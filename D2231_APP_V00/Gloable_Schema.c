/*
 * Gloable_Schema.c
 *
 * 全局变量及头文件
 *
 *  Created on: 2025-12-22
 *      Author: Administrator
 */
#include "Gloable_Schema.h"

#define RS485_NUM   4

volatile int fd_RS485_115200[RS485_NUM] = {0};//3:轨道

void Set_fd_RS485_115200(int _index, int val)
{
	fd_RS485_115200[_index] = val;
}

int Get_fd_RS485_115200(int _index)
{
	return fd_RS485_115200[_index];
}

volatile queue *PQueue_TarskControlModule1 = NULL;//轨道模块队列
volatile queue *PQueue_ManipulatorModule1 = NULL;//机械手模块队列
volatile queue *PQueue_TurnPlate_1_Module = NULL;//转盘1模块队列
volatile queue *PQueue_TurnPlate_2_Module = NULL;//转盘2模块队列
volatile queue *PQueue_TurnPlate_3_Module = NULL;//转盘3模块队列
volatile queue *PQueue_SampleShelf_Module = NULL;//样本架控制模块队列
volatile queue *PQueue_ControlBoard_1_Module = NULL;//分控1模块队列
volatile queue *PQueue_ControlBoard_2_Module = NULL;//分控2模块队列
volatile queue *PQueue_ControlBoard_3_Module = NULL;//分控3模块队列
volatile queue *PQueue_ControlBoard_4_Module = NULL;//分控4模块队列

State_Moudle _State_Moudle;

volatile queue *PQueue_ETH_Send = NULL;//网口发送队列

/*
 * 命令执行完成返回的队列，入队后当队列不为空时发送
 * action_index : 动作索引 若为组合动作，出错时，返回出错时动作的索引，从1开始；其它情况，包括正常状态返回0。
 * module_index ： 组件索引 默认为FF，保留。
 * num ：附加参数长度,第一个附加参数为状态码
 */

int Eth_Send_Queue(NetCmd *cmd, int action_index, int module_index, int num, ...)
{
	va_list args;
	char send_buf[4096] = "";
	char send_crcbuf[8] = "";
	char ActionIndex[2] = "";
	char ModuleIndex[4] = "";
	memset(send_buf, 0, sizeof(send_buf));
	strncat(send_buf, cmd->netcmdhead, 11);
	strcat(send_buf, "02");
	//strcat(send_buf, cmd->netcmdhead + 11);
	sprintf(ActionIndex, "%X", action_index);
	sprintf(ModuleIndex, "%02X", module_index);
	strcat(send_buf, ActionIndex);
	strcat(send_buf, ModuleIndex);
	va_start(args, num);
	char stateNum[8];
	sprintf(stateNum, "%04X", va_arg(args, int));
	strcat(send_buf, stateNum);
	int num_t = 5 - num;
	num = num - 1;
	while(num-- > 0)
	{
		strcat(send_buf, va_arg(args, char*));
	}
	if(cmd->code != 0x07 && cmd->code != 0x08 && cmd->code != 0x83 && cmd->code != 0x84  && cmd->code != 0x85)
	{
		while(num_t-- > 0)
		{
			strcat(send_buf, "00000000");
		}
	}
	crc_check((uint8_t *)send_buf, send_crcbuf);
	strcat(send_buf, send_crcbuf);
	strcat(send_buf, "\r\n");

	va_end(args);
	return enQueue(PQueue_ETH_Send, send_buf, strlen(send_buf));;
}

void InitAsyncCmdlist(AsyncCmdlist *list)
{
	list -> head = NULL;
	list -> tail = NULL;
}

void ClearAsyncCmdList(AsyncCmdlist *list)
{
	AsyncCmdNode *temp;
	while(list -> head != NULL)
	{
		temp = list -> head;
		list -> head = list -> head -> next;
		free(temp);
	}
	list -> tail = NULL;
}

void putInAsyncCmd(AsyncCmdlist *list, NetCmd cmd)
{
	AsyncCmdNode *pNew = (AsyncCmdNode *)malloc(sizeof(AsyncCmdNode));
    pNew -> next = NULL;
    int i;
    memset(pNew->cmd.pvar, 0, NET_CMD_LENTH_MSG);
    pNew->cmd = cmd;
    if(list -> head != NULL)
    {
        list -> tail -> next = pNew;
        list -> tail = pNew;
    }
    else
        list -> head = list -> tail = pNew;
}

/**
 * 1 - 找到
 * 0 - 未找到
 */
char takeOutAsyncCmd(AsyncCmdlist *list, unsigned char viraddr, unsigned char code)
{
	AsyncCmdNode *pPre = list -> head;
	AsyncCmdNode *pTemp = list -> head;

    while(pTemp != NULL)
    {
        if(viraddr == pTemp->cmd.viraddr && code == pTemp->cmd.code)
        {
        	Eth_Send_Queue(&pTemp->cmd, 0, 0xFF, 1, 0x0000);

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

void my_memset(int *str, int vle, int len)
{
	while(len--)
	{
		*str++ = vle;
	}
}

//动作编号
volatile int ManipulatorModule_Num = STEP_0;
volatile int TurnPlate_1_Module_Num = STEP_0;
volatile int TurnPlate_2_Module_Num = STEP_0;
volatile int TurnPlate_3_Module_Num = STEP_0;
volatile int SampleShelf_Module_Num = STEP_0;
volatile int ControlBoard_Motor_Num[4] = {STEP_0};//[分控]
volatile int SampleShelf_Scan_Succeed = 0; 		/*样品架扫码成功*/


int reset = -1;

NetCmd *_NetCmd_reset = NULL;

ClawsPos *My_ClawsPos;
SampleTurntablePos *turntableSamplePos;
TransportTurntablePos *turntable2Pos;
TransportTurntablePos *railTurntablePos;
NetInfo *_NetInfo;

/*
 * 保存位置信息
 */
char SavePointConfigure(char *filename, NetCmd *cmd)
{
	//写入之前判断文件是否存在
	FILE *fd;
	char *a="[POINT]\n02=\n";
	if(NULL == (fd = fopen(filename, "r")))
	{
		fd = fopen(filename, "w");
		fputs(a,fd);
		fclose(fd);
	}else{
		fclose(fd);
		char point_key[32][8] = {"02", "04", "06", "07", "0A",
				"27", "23", "13", "12", "02", "22"};
		int i = 0;
		int s = 0;
		for(i = 0; i < 11; i++)
		{
			s = FindIniKey("POINT", point_key[i], filename);
			if(s == 1)
			{
				//找到 title和key
			}else if(s == 0 || s == -1)//均未找到 / 未找到文件
			{
				fd = fopen(filename, "w");
				fputs(a,fd);
				fclose(fd);
				fd = NULL;
				break;
			}else if(s == 2)//找到title但未找到key
			{
				fd = fopen(filename, "a+");
				char buf_tmp[8] = {""};
				sprintf(buf_tmp, "%s=\r\n", point_key[i]);
				fputs(buf_tmp, fd);
				fclose(fd);
				fd = NULL;
			}
		}
	}
	switch(cmd->viraddr){
	case 0x02:
	{
		unsigned int pvarlength0x02 = PackWord((char*) &cmd->pvar[2]) * 2;
		char ch0x02[4096] = "";
		memset(ch0x02, 0, sizeof(ch0x02));
		memcpy(ch0x02, (char*)&cmd->pvar[2], pvarlength0x02 + 4);
		PutIniKeyString("POINT", "02", ch0x02, filename);
	}
		break;
	case 0x04:
	{
		unsigned int pvarlength0x04 = PackWord((char*) &cmd->pvar[2]) * 2;
		char ch0x04[4096] = "";
		memset(ch0x04, 0, sizeof(ch0x04));
		memcpy(ch0x04, (char*)&cmd->pvar[2], pvarlength0x04 + 4);
		PutIniKeyString("POINT", "04", ch0x04, filename);
	}
		break;
	case 0x06:
	{
		unsigned int pvarlength0x06 = PackWord((char*) &cmd->pvar[2]) * 2;
		char ch0x06[4096] = "";
		memset(ch0x06, 0, sizeof(ch0x06));
		memcpy(ch0x06, (char*)&cmd->pvar[2], pvarlength0x06 + 4);
		PutIniKeyString("POINT", "06", ch0x06, filename);
	}
		break;
	case 0x07:
	{
		unsigned int pvarlength0x06 = PackWord((char*) &cmd->pvar[2]) * 2;
		char ch0x06[4096] = "";
		memset(ch0x06, 0, sizeof(ch0x06));
		memcpy(ch0x06, (char*)&cmd->pvar[2], pvarlength0x06 + 4);
		PutIniKeyString("POINT", "07", ch0x06, filename);
	}
		break;
	}
	return 0;
}

/*
 * 读取位置信息
 */
char *ReadPointConfigure(char *title, char *key, char *filename, char *buf)
{
	FILE *fd;
	if(NULL == (fd = fopen(filename, "r")))
	{
		return NULL;
	}else
	{
		fclose(fd);
	}
	char *buf_tmp = GetIniKeyString(title, key, filename);
	if(buf_tmp != NULL)
	{
		strcpy(buf, buf_tmp);
	}
	return buf;
}



Moudle_Point _Moudle_Point;

/*
 * start 开始时间
 * now   当前时间
 * timeout_period 超时时间（ms）
 *
 * return：
 * -1 未超时
 * 0  超时
 */
int My_timeout(struct timeval *start, struct timeval *now, int timeout_period)
{
	if((1000000 * (now->tv_sec - start->tv_sec) + now->tv_usec - start->tv_usec) > (timeout_period * 1000))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}
NetCmd *cmd_AutoSend_SampleShelf_State = NULL;//电磁铁板状态有变化自动上返
NetCmd *cmd_AutoSend_Sensor_State = NULL;//传感器1触发自动上返
int calc_diff_bit(int m, int n)
{
    int tmp = m ^ n;
    return tmp;
}

char BarCode_buf[32] = {0};//样品架条码buf
char BarCode_buf_len[9] = {0};//长度

char BarCode_recv_finish_1 = 0;//是否可以发送条码给上位机，0-不发送，不为0发送
char BarCode_recv_finish_2 = 0;//是否可以发送条码给上位机，0-不发送，不为0发送

int state1[6] = {0x0001, 0x0000, 0x0003, 0x0000, 0x0004, 0x0000};

volatile int ManipulatorModule_Area = 0;

int Get_ManipulatorModule_Area()
{
	printf("Get_ManipulatorModule_Area = %d\r\n", ManipulatorModule_Area);
	return ManipulatorModule_Area;
}

void Set_ManipulatorModule_Area(int val)
{
	ManipulatorModule_Area = val;
	printf("Set_ManipulatorModule_Area = %d\r\n", ManipulatorModule_Area);
}

message _message_TurnPlate_1_1;


int CarID_Can_Return = 0;//表示转盘1 3位置的小车ID已经上返过了。在读IO的时候用，在转盘从3转走以后清除标志,为0表示可以上返

void Set_CarID_Can_Return(int val)
{
	CarID_Can_Return = val;
}

int Get_CarID_Can_Return()
{
	return CarID_Can_Return;
}

int Device_Mode = 1; //仪器模式

void Set_Device_Mode(int val)
{
	Device_Mode = val;
}

int Get_Device_Mode()
{
	return Device_Mode;
}

char IsTurntableRespond = 1;

void SetTurntableRespond(int val)
{
	IsTurntableRespond = val;
}

char GetTurntableRespond()
{
	return IsTurntableRespond;
}

//待检小车总数量 (小车自检模式)
int Car_Amount = 0;
int Car_Done_Amount = 0;

void Set_Car_Amount(int val)
{
	Car_Amount = val;
	Car_Done_Amount = 0;
}

int Get_Car_Amount()
{
	return Car_Amount;
}

void Car_Done_Plus()
{
	Car_Done_Amount++;
}

int Get_Car_Done_Amount()
{
	return Car_Done_Amount;
}

int EmptyCar_IntoTark = 0; //空小车是否进入轨道  1-不进入 0-进入

void Set_EmptyCar_IntoTark(int val)
{
	EmptyCar_IntoTark = val;
}

int Get_EmptyCar_IntoTark()
{
	return EmptyCar_IntoTark;
}

int Trak_State = 0; //履带状态  1-启动 0-未启动

void Set_Trak_State(int val)
{
	Trak_State = val;
}

int Get_Trak_State()
{
	return Trak_State;
}

int Branch_State[4] = {0}; //分支状态  1-启用 0-未启用

void Set_Branch_State(int branch, int val)
{
	Branch_State[branch] = val;
}

int Get_Branch_State(int branch)
{
	return Branch_State[branch];
}

int SaveToFile(char *filename, char *buf)
{
    FILE *fpw;
    if (NULL == (fpw = fopen(filename,    "w")))
    {
    	perror("fopen");//
    	fpw = NULL;
    	return -2;
    }
    fputs(buf, fpw); // 写入文件

    fclose(fpw);
    fpw = NULL;
    return 0;
}

int ReadForFile(char *filename, char *buf)
{
    FILE *fpr;
    if (NULL == (fpr = fopen(filename,    "r")))
    {
    	perror("fopen");//
    	fpr = NULL;
    	return -2;
    }
    fgets(buf, 20, fpr); // 读文件

    fclose(fpr);
    fpr = NULL;
    return 0;
}

datetime_t _dt;

//绝对时间解包
void datetime_unpack_hex(const uint64_t v, datetime_t *dt)
{
    dt->year   = (v >> 26) & 0xFF;
    dt->month  = (v >> 22) & 0x0F;
    dt->day    = (v >> 17) & 0x1F;
    dt->hour   = (v >> 12) & 0x1F;
    dt->minute = (v >> 6)  & 0x3F;
    dt->second =  v      & 0x3F;
}
//绝对时间组包
uint64_t datetime_pack_bits(const datetime_t *dt)
{
    uint64_t v = 0;
    v |= ((uint64_t)(dt->year   & 0xFF)) << 26;
    v |= ((uint64_t)(dt->month  & 0x0F)) << 22;
    v |= ((uint64_t)(dt->day    & 0x1F)) << 17;
    v |= ((uint64_t)(dt->hour   & 0x1F)) << 12;
    v |= ((uint64_t)(dt->minute & 0x3F)) << 6;
    v |= ((uint64_t)(dt->second & 0x3F));
    return v;
}

void tmTodatetime_t(struct tm * stime, datetime_t *dt)
{
    dt->year   = (uint8_t)stime->tm_year - (2000 - 1900);
    dt->month  = (uint8_t)stime->tm_mon + 1;
    dt->day    = (uint8_t)stime->tm_mday;
    dt->hour   = (uint8_t)stime->tm_hour;
    dt->minute = (uint8_t)stime->tm_min;
    dt->second = (uint8_t)stime->tm_sec;
}

uint32_t Track_Motor_Exist[4] = {0};
