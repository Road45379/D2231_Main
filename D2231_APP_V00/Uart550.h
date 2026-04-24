/**
 ------------------------------------------------------------------
 This driver file is applicable to the following models of devices:
 (WR-4，WR-6，WR-7）
 ------------------------------------------------------------------
 * Copyright (C), XXX. Co., Ltd.
 * FileName: Uart550.h
 * Description:     //ZYNQ Uart550 操作接口
 * Version:         // 1.0
 * Function List:
 1. -------Uart16550_Open
 2. -------Uart16550_Send
 3. -------Uart16550_Close
 * History:
 <author>  <time>   <version >   <desc>
 Kyle    24/07/24     1.0     init
 */
#ifndef UART550_H_
#define UART550_H_
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <stdint.h>
#include <sys/mman.h>

typedef enum
{
	stopBits_1 = 0, stopBits_2 = 1,
} stopBits;
typedef enum
{
	Parity_None = 0,//无校验
	Parity_Odd = 1,//奇校验
	Parity_Even = 3,//偶校验
	Parity_Mark = 5,//标志校验
	Parity_Sapce = 7,
//空格校验
} Parity;

#define Rs485Mode 1
#define Rs422Mode 0
#define Rs485Line 6

/**
 * Function:       // Uart16550_Open
 * Description:    // 特定串口初始化
 * Input:* uint32_t PhyAddress,   //串口的物理地址 由硬件决定
 * int32_t IRQ,//串口的中断号 由硬件决定
 * int32_t BitRate,  //比特率 常见的有 9600 19200 38400 57600 115200 等等 根据需要填写
 * stopBits st,//停止位  [stopBits_1 表示一位停止位]     [stopBits_2  表示两位停止位]
 * Parity Pra,  //校验位  [Parity_None 没有校验]          [Parity_Odd  奇校验]             [Parity_Even 偶校验]
 * int8_t Is485Mode  //是否为485模式  0 为正常模式其他数为RS485模式
 * int32_t _485Line, //当为485模式时EMIO的线序号
 * void(*func_rcv)(unsigned char,int)//回调函数
 * int32_t id //回调函数回调时的第二个参数 不用则为0
 * Output:         // 无
 * Return:         // 成功则返回设备对应的设备号 否则返回-1
 * Others:         //无
 */
int32_t Uart16550_Open(uint32_t PhyAddress, int32_t IRQ, int32_t BitRate,
		stopBits st, Parity Pra, int8_t Is485Mode, int32_t _485Line,
		void(*func_rcv)(uint8_t, int32_t), int32_t usrdat);
/**
 * Function:       // Uart16550_Send
 * Description:    // 串口数据发送
 * Input:* int32_t fd ,   //Uart16550_Open 返回的句柄
 * uint8_t*buffer,//待发送数据缓存
 * int32_t len,  //待发送数据长度
 * Output:         // 无
 * Return:         // 正数表示成功放入缓存的数据长度    负数表示异常
 * Others:         //无
 */
int32_t Uart16550_Send(int32_t fd, uint8_t*buffer, int32_t len);
/**
 * Function:       // Uart16550_Close
 * Description:    // 关闭串口
 * Input:* int32_t fd ,   //Uart16550_Open 返回的句柄
 * Output:         // 无
 * Return:         // 无
 * Others:         //无
 */
void Uart16550_Close(int32_t fd);

#endif /* UART550_H_ */
