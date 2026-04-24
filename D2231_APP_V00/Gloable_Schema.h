/*
 * Gloable_Schema.h
 *
 *  Created on: 2025-12-22
 *      Author: Administrator
 */
#ifndef GLOABLE_SCHEMA_H_
#define GLOABLE_SCHEMA_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include "Myqueue.h"
#include "CRC.h"
#include <sys/socket.h>
#include "Uart550.h"
#include "UART_User_Code.h"
#include "ASCII_Transition.h"
#include "ETH_Communication.h"
#include "Ini_handle.h"
#include "CAN_User_Code.h"
#include "ErrorCode.h"

#define BIT(num,n) 			((num & (1 << n)) > 0)//判断第n位
#define getBit(num, n) 		((num >> n) & 1) //获取第N位
#define get2Bit(num, n) 	((num >> (n*2)) & 0x11) //获取第N位开始的2BIT

#define BIT_Set(num,n) 		num = (num | (1 << n))//设置第n位为1
#define BIT_Clear(num,n) 	num = (num & ~(1 << n))//设置第n位为0

#define COMMAND_ADD(com)  com - 'A'

#define Filename_config  "/emmc/config.ini"
#define Filename_IP  "/emmc/IP.ini"
#define Filename_MAC  "/fs0p1/mac.ini"

#define PublicCommandAdd						0xFF            /*公共指令集*/
#define TarskControlCommandAdd					0x05            /*轨道指令*/
#define ManipulatorModuleCommandAdd				0x02            /*机械手指令*/
#define SampleShelf_ModuleCommandAdd			0x03            /*样本架控制指令*/
#define TurnPlate_1_ModuleCommandAdd			0x04            /*转盘1指令*/
#define TurnPlate_2_ModuleCommandAdd			0x06            /*转盘2指令*/
#define TurnPlate_3_ModuleCommandAdd			0x07            /*转盘3指令*/
#define ControlBoard_1_ModuleCommandAdd			0x08            /*分控1指令*/
#define ControlBoard_2_ModuleCommandAdd			0x09            /*分控2指令*/
#define ControlBoard_3_ModuleCommandAdd			0x0A            /*分控3指令*/
#define ControlBoard_4_ModuleCommandAdd			0x0B            /*分控4指令*/

//电机状态
#define State_Busy								0x00			/*忙*/
#define State_NoBusy							0x01			/*空闲*/
#define State_Error								0x02			/*出错*/
#define State_Wait								0x03			/*等待*/
#define State_Crash								0x08			/*碰撞*/

extern volatile queue *PQueue_TarskControlModule1;//轨道模块队列
extern volatile queue *PQueue_ManipulatorModule1;//机械手模块队列
extern volatile queue *PQueue_TurnPlate_1_Module;//转盘1模块队列
extern volatile queue *PQueue_TurnPlate_2_Module;//转盘1模块队列
extern volatile queue *PQueue_TurnPlate_3_Module;//转盘1模块队列
extern volatile queue *PQueue_SampleShelf_Module;//样本架控制模块队列
extern volatile queue *PQueue_ControlBoard_1_Module;//分控1模块队列
extern volatile queue *PQueue_ControlBoard_2_Module;//分控2模块队列
extern volatile queue *PQueue_ControlBoard_3_Module;//分控3模块队列
extern volatile queue *PQueue_ControlBoard_4_Module;//分控4模块队列

extern volatile queue *PQueue_ETH_Send;//网口发送队列

#define fd_RS485_index_0   0//样本架控制模块（电磁铁板）+ 所有分控
#define fd_RS485_index_1   1//转盘1+2个RFID
#define fd_RS485_index_2   2//机械手
#define fd_RS485_index_3   3//轨道
void Set_fd_RS485_115200(int _index, int val);
int Get_fd_RS485_115200(int _index);



#define NET_CMD_LENTH_MSG           4096   /*NET命令数据长度*/
typedef struct _NetCmd
{
	int cmdid;                      		/*命令id*/
	unsigned char viraddr;					/*模块地址*/
	unsigned char code;						/*功能码*/
	char pvar[NET_CMD_LENTH_MSG];			/*参数*/
	short cnt;								/*参数字节数*/
	char paramtype;							/*0数组  1指针*/
	char netcmdhead[256];
}NetCmd;

typedef struct _AsyncCmdNode{
	NetCmd cmd;
	struct _AsyncCmdNode *next;
}AsyncCmdNode;

typedef struct _AsyncCmdlist{
	AsyncCmdNode * head;
	AsyncCmdNode * tail;
}AsyncCmdlist;

AsyncCmdlist asyncCmdlist;

typedef volatile struct State_Moudle{
	volatile int State_TarskControlModule1;//轨道模块状态
	volatile int State_Manipulator;//机械手模块状态
	volatile int State_DeviceReset;//整机复位状态
	volatile int State_TurnPlate_1_Module;//转盘1状态
	volatile int State_TurnPlate_2_Module;//转盘2状态
	volatile int State_TurnPlate_3_Module;//转盘3状态
	volatile int State_SampleShelf_Module;//样本架控制模块状态
	volatile int State_ControlBoard_Module[4];//[分控]状态

}State_Moudle;
extern State_Moudle _State_Moudle;

int Eth_Send_Queue(NetCmd *cmd, int action_index, int module_index, int num, ...);

void my_memset(int *str, int vle, int len);

#define MOTOR_0_STEPS               "00000000"//电机0位置，电机回0的时候发

//电机命令
#define MOTOR_COM_READ_VERSION      'A'//读版本
#define MOTOR_COM_MOMENT_MODE       'I' //电机力矩模式 （履带电机启停用此指令）
#define MOTOR_COM_FREE				'a' //解锁/锁定电机
#define MOTOR_COM_READSTEPS			'E' //读电机位置
#define MOTOR_COM_SETSTEPS			'D'	//设置电机位置
#define MOTOR_COM_READ_IN_PLACE		'd' //询问是否到位
#define MOTOR_COM_RESET				'G'	//电机复位
#define MOTOR_COM_READRESETSTATE	'g'	//读电机复位状态
#define MOTOR_COM_MOTOR_START       'K' //电机启动
#define MOTOR_COM_READ_PARA      	'X' //读电机参数
#define MOTOR_COM_WRITE_PARA      	'Y' //写电机参数
#define MOTOR_COM_SAVE_PARA      	'U' //保存电机参数
#define MOTOR_COM_READ_STATE      	'Q' //读电机状态

//电爪命令 （电爪和电机公用的命令按照电机命令的定义）
#define HAND_COM_READSTEPS			'I' //读电机位置
#define HAND_COM_READSTATE			'd' //读d电爪状态
#define HAND_COM_MOVEMENT			'E' //抓/放
	#define HAND_COM_MOVEMENT_TAKEUP	"1" //抓
	#define HAND_COM_MOVEMENT_PUTDOWN	"2" //放

//电磁铁板命令
#define MAGENT_COM_READ_VERSION     'D'//读版本
#define MAGENT_COM_READSTEPS		'E' //查询当前微动开关、灯、案件状态
#define MAGENT_COM_LED_CONTROL		'F' //灯控制

//RFID命令

#define READ_RFID   	'T'//读RFID
#define WRITE_RFID   	'R'//写RFID

//主控的分控命令（can）
#define Read_Version							0x01			/*读版本*/
#define OpenOrCloseScan_Command					0x3D			/*打开/关闭扫码枪*/
#define Scan_Recv_Command						0x3E			/*扫码自动上返*/
#define OpenOrCloseScan_2_Command				0x4D			/*打开/关闭流水线扫码枪*/
#define Scan_Recv_2_Command						0x4E			/*流水线扫码自动上返*/
#define OpenBuzzer_Command      				0x4F			/*打开蜂鸣器*/

//分控命令
#define ControlBoard_Get_State_Command				'0'			//分控心跳包
#define ControlBoard_Motor_Para_Command				'1'			//读写电机参数
#define ControlBoard_Motor_Reset_Command			'2'			//分控电机复位
#define ControlBoard_FreeMortor_Command				'3'			//分控/锁定/释放电机
#define ControlBoard_Set_Motor_Place_Command		'4'			//分控读写电机坐标
#define ControlBoard_ReadOrWrite_Para_Command		'5'			//分控读写参数
#define ControlBoard_Set_Mode_Command				'6'			//分控设置整机模式
#define ControlBoard_Read_Version_Command			'7'			//分控读版本
#define ControlBoard_92_Can_Test_Project_Command	'8'			//92可测试项目配置
#define ControlBoard_Branch_Free_Command			'A'			//分控分支释放
#define ControlBoard_Turntable_Respond_Command		'B'			//转盘是否响应

//上位机命令
#define READ_VERSION				0x01//读版本



extern int state1[6];

//动作编号
enum{
	STEP_0,
	STEP_1,
	STEP_2,
	STEP_3,
	STEP_4,
	STEP_5,
	STEP_6,
	STEP_7,
	STEP_8,
	STEP_9,
	STEP_10,
	STEP_11,
	STEP_12
};
extern volatile int ManipulatorModule_Num;
extern volatile int TurnPlate_1_Module_Num;
extern volatile int TurnPlate_2_Module_Num;
extern volatile int TurnPlate_3_Module_Num;
extern volatile int SampleShelf_Module_Num;
extern volatile int ControlBoard_Motor_Num[4];//[分控]
extern volatile int SampleShelf_Scan_Succeed; 		/*样品架扫码成功*/

void ParsingStringToNetCmd(char *buf, NetCmd *_NetCmd);

extern int reset;

extern NetCmd *_NetCmd_reset;

//+++++++++++++++++++++++++++++++++++++机械臂结构体
typedef struct _PosCoordinate
{
    int x;//表示x轴方向
    int y;//表示y轴方向
    int z;//表示垂直方向
}PosCoordinate;

typedef struct _PosInterval //位置间距
{
    int xHoleInterval;//表示孔x轴方向间距
    int yHoleInterval;//表示孔y轴方向间距
}PosInterval;

typedef struct _ClawSampleRackPos//样品架
{
    PosCoordinate startBasePos;//样品架1孔
    PosCoordinate scanFrameCodePos;//样品架扫码位置
}ClawSampleRackPos;

typedef struct _ClawsPosSampleArea //样品区
{
    ClawSampleRackPos clawRackPos[6];//6个样品架
    PosInterval   interval;//间距
}ClawsPosSampleArea ;

typedef struct _ClawsPosEmergencyRackArea//急诊
{
    PosCoordinate startBasePos[4];//4个样品架
    PosInterval   interval;
}ClawsPosEmergencyArea;

typedef struct _ClawsPos //电爪定位信息
{
    ClawsPosSampleArea  sampleAreaPos;
    ClawsPosEmergencyArea  emergencyAreaPos;
    PosCoordinate enterTrackPos; //样品放入流水线的位置
    PosCoordinate unloadTrackPos; //样品从流水线卸载的位置
}ClawsPos;
extern ClawsPos *My_ClawsPos;

//+++++++++++++++++++++++++++++++++++++转盘结构体
typedef struct _SampleTurntablePos
{
    int portPos[3];
    uint16_t waitingTime[2];
}SampleTurntablePos;

typedef struct _TransportTurntablePos
{
    int portPos[4];
    uint16_t waitingTime[4];
}TransportTurntablePos;

typedef struct _NetInfo{
    //8字符
    uint8_t ip[4];
    uint8_t mac[6];
}NetInfo;

extern SampleTurntablePos *turntableSamplePos;//进出样转盘
extern TransportTurntablePos *turntable2Pos;
extern TransportTurntablePos *railTurntablePos;//进出轨道转盘
extern NetInfo *_NetInfo;//进出样转盘

char SavePointConfigure(char *filename, NetCmd *cmd);
char *ReadPointConfigure(char *title, char *key, char *filename, char *buf);

typedef volatile struct Moudle_Point
{
	volatile int ManipulatorModule_SrcArea;
	volatile int ManipulatorModule_SrcHole;
	volatile int ManipulatorModule_point_x;
	volatile int ManipulatorModule_point_y;
	volatile int ManipulatorModule_point_z;
	volatile int TurnPlate_1_Module_point;
	volatile int TurnPlate_2_Module_point;
	volatile int TurnPlate_3_Module_point;
	volatile int TurnPlate_1_Module_NowHole;//转盘1当前位置
}Moudle_Point;

extern Moudle_Point _Moudle_Point;

int My_timeout(struct timeval *start, struct timeval *now, int timeout_period);

extern NetCmd *cmd_AutoSend_SampleShelf_State;//电磁铁板状态有变化自动上返
extern NetCmd *cmd_AutoSend_Sensor_State;//传感器1触发自动上返

int calc_diff_bit(int m, int n);

extern char BarCode_buf[32];//样品架条码buf
extern char BarCode_buf_len[9];//长度
extern char BarCode_recv_finish_1;//是否可以发送条码给上位机，0-不发送，不为0发送
extern char BarCode_recv_finish_2;//是否可以发送条码给上位机，0-不发送，不为0发送

int Get_ManipulatorModule_Area();
void Set_ManipulatorModule_Area(int val);

typedef struct{
	int _04_hand;
	int _04_offset;
	char state;//读卡的状态 （01，02寻卡失败，08成功）
	uint8_t BarCode_len;//条码长度
	uint16_t message_len;//测试信息长度
	char Reserve[2];//保留
	uint32_t CAR_ID;
	uint32_t Wirte_Num;//写入次数
	char BarCode[16];//条码长度
	char message[192];//测试信息
	char tarck[96];//轨迹信息
}message;

extern message _message_TurnPlate_1_1;


void Set_CarID_Can_Return(int val);
int Get_CarID_Can_Return();

void Set_Device_Mode(int val);
int Get_Device_Mode();

void SetTurntableRespond(int val);
char GetTurntableRespond();

void Car_Done_Plus();
int Get_Car_Done_Amount();

void Set_Car_Amount(int val);
int Get_Car_Amount();

void Set_EmptyCar_IntoTark(int val);
int Get_EmptyCar_IntoTark();

void Set_Trak_State(int val);
int Get_Trak_State();

void Set_Branch_State(int branch, int val);
int Get_Branch_State(int branch);

int SaveToFile(char *filename, char *buf);
int ReadForFile(char *filename, char *buf);

typedef struct {
    uint8_t year;    // year从2000开始，范围 0–255
    uint8_t month;   // 1–12
    uint8_t day;     // 1–31
    uint8_t hour;    // 0–23
    uint8_t minute;  // 0–59
    uint8_t second;  // 0–59
} datetime_t;

extern datetime_t _dt;

void tmTodatetime_t(struct tm * stime, datetime_t *dt);
uint64_t datetime_pack_bits(const datetime_t *dt);
void datetime_unpack_hex(const uint64_t v, datetime_t *dt);

void InitAsyncCmdlist(AsyncCmdlist *list);
void ClearAsyncCmdList(AsyncCmdlist *list);
void putInAsyncCmd(AsyncCmdlist *list, NetCmd cmd);
char takeOutAsyncCmd(AsyncCmdlist *list, unsigned char viraddr, unsigned char code);

extern uint32_t Track_Motor_Exist[4];

#endif //GLOABLE_SCHEMA_H_
