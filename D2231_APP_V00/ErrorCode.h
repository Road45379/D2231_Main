/*
 * ErrorCode.h
 *
 *  Created on: 2026-1-26
 *      Author: ThinkPad
 */

#ifndef ERRORCODE_H_
#define ERRORCODE_H_
#include <stdint.h>
//机械手
#define ManipulatorModule_X_Communication_Err      0x01//通讯失败
#define ManipulatorModule_X_Reset_Err      		   0x02//复位失败
#define ManipulatorModule_X_Collision      		   0x03//碰撞
#define ManipulatorModule_Y_Communication_Err      0x04//通讯失败
#define ManipulatorModule_Y_Reset_Err      		   0x05//复位失败
#define ManipulatorModule_Y_Collision      		   0x06//碰撞
#define ManipulatorModule_Z_Communication_Err      0x07//通讯失败
#define ManipulatorModule_Z_Reset_Err      		   0x08//复位失败
#define ManipulatorModule_Z_Collision      		   0x09//碰撞
#define ManipulatorModule_HAND_Communication_Err   0x0A//通讯失败
#define ManipulatorModule_HAND_Reset_Err      	   0x0B//复位失败
#define ManipulatorModule_HAND_Collision      	   0x0C//电爪未夹到试管
#define ManipulatorModule_TUBE_FELL				   0x0D//机械臂运行时试管掉落
#define ManipulatorModule_OPENBARCODE_Err		   0x0E//打开样品架扫码枪失败
#define ManipulatorModule_SCANCODES_Err			   0x0F//未扫描到样品架条码

//样品架
#define SampleShelfModule_Communication_Err        		0x01//与电磁铁板通讯失败
#define SampleShelfModule_Buzzer_Communication_Err      0x02//蜂鸣器控制板通讯失败

//转盘1
#define TurnPlate_1_Module_Communication_Err        		0x01//转盘1通讯失败
#define TurnPlate_1_Module_Reset_Err        				0x02//转盘1复位失败
#define TurnPlate_1_Module_Collision        				0x03//转盘1碰撞
#define TurnPlate_1_Module_Unload_RFID_Communication_Err    0x04//转盘1卸载点RFID通信失败
#define TurnPlate_1_Module_Unload_RFID_FindCard_Err    		0x05//转盘1卸载点RFID寻卡失败
#define TurnPlate_1_Module_Load_RFID_Communication_Err    	0x06//转盘1加样点RFID通信失败
#define TurnPlate_1_Module_Load_RFID_FindCard_Err    		0x07//转盘1加样点RFID寻卡失败
#define TurnPlate_1_Module_R_Motor_Communication_Err   		0x08//转盘1旋转电机通信失败
#define TurnPlate_1_Module_OPENBARCODE_Err   				0x0A//打开扫码枪失败
#define TurnPlate_1_Module_SCANCODES_Err   					0x0B//未扫到条码

//转盘2
#define TurnPlate_2_Module_Communication_Err        		0x01//转盘2通讯失败
#define TurnPlate_2_Module_Reset_Err        				0x02//转盘2复位失败
#define TurnPlate_2_Module_Collision        				0x03//转盘2碰撞
#define TurnPlate_2_Module_RFID_Communication_Err    		0x04//转盘2附近RFID通信失败
#define TurnPlate_2_Module_RFID_FindCard_Err    			0x05//转盘2附近RFID寻卡失败

//转盘3
#define TurnPlate_3_Module_Communication_Err        		0x01//转盘3通讯失败
#define TurnPlate_3_Module_Reset_Err        				0x02//转盘3复位失败
#define TurnPlate_3_Module_Collision        				0x03//转盘3碰撞
#define TurnPlate_3_Module_RFID_Communication_Err    		0x04//转盘3附近RFID通信失败
#define TurnPlate_3_Module_RFID_FindCard_Err    			0x05//转盘3附近RFID寻卡失败

//分控
#define ControlBoard_Mode_Communication_Err					0x0F//分控通讯失败


//传送带电机错误码不重新定义，错误码为05 + 电机地址


typedef struct{
	uint8_t Communication_Err;//通讯失败
	uint8_t Reset_Err;//复位失败
	uint8_t Collision;//碰撞（电爪使用时表示未夹到试管）
}Motor_Err;

extern Motor_Err ManipulatorModule_Motor_Err[4];

int CompoundErrorCode(int ModuleNum, int ErrorNum);

#endif /* ERRORCODE_H_ */
