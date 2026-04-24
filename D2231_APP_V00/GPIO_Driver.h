/*
 * GPIO_Driver.h
 *
 *  Created on: 2019-11-28
 *      Author: ThinkPad
 */

#ifndef GPIO_DRIVER_H_
#define GPIO_DRIVER_H_

#include <stdint.h>
#include <sys\mman.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/neutrino.h>
#include <stdint.h>

struct GPIO_Reg {
	volatile int MASK_DATA_0_LSW;//00
	volatile int MASK_DATA_0_MSW;//04
	volatile int MASK_DATA_1_LSW;//08
	volatile int MASK_DATA_1_MSW;//0C
	volatile int MASK_DATA_2_LSW;//10
	volatile int MASK_DATA_2_MSW;//14
	volatile int MASK_DATA_3_LSW;//18
	volatile int MASK_DATA_3_MSW;//1C
	volatile int Rev1[8];//20-3C
	volatile int DATA_0;//40
	volatile int DATA_1;//44
	volatile int DATA_2;//48
	volatile int DATA_3;//4C
	volatile int Rev2[4];//50-5c
	volatile int DATA_0_RO;//60
	volatile int DATA_1_RO;//64
	volatile int DATA_2_RO;//68
	volatile int DATA_3_RO;//6C
	volatile int Rev3[101];//70-200
	volatile int DIRM_0;//204
	volatile int OEN_0;//208
	volatile int INT_MASK_0;//20C
	volatile int INT_EN_0;//210
	volatile int INT_DIS_0;//214
	volatile int INT_STAT_0;//218
	volatile int INT_TYPE_0;//21C
	volatile int INT_POLARITR_0;//220
	volatile int INT_ANY_0;//224
	volatile int Rev4[7];//
	volatile int DIRM_1;//244
	volatile int OEN_1;//
	volatile int INT_MASK_1;//
	volatile int INT_EN_1;//
	volatile int INT_DIS_1;//
	volatile int INT_STAT_1;//
	volatile int INT_TYPE_1;//
	volatile int INT_POLARITR_1;//
	volatile int INT_ANY_1;//264
	volatile int Rev5[7];//
	volatile int DIRM_2;//284
	volatile int OEN_2;//
	volatile int INT_MASK_2;//
	volatile int INT_EN_2;//
	volatile int INT_DIS_2;//
	volatile int INT_STAT_2;//
	volatile int INT_TYPE_2;//
	volatile int INT_POLARITR_2;//
	volatile int INT_ANY_2;//2A4
	volatile int Rev6[7];//
	volatile int DIRM_3;//2C4
	volatile int OEN_3;//
	volatile int INT_MASK_3;//
	volatile int INT_EN_3;//
	volatile int INT_DIS_3;//
	volatile int INT_STAT_3;//
	volatile int INT_TYPE_3;//
	volatile int INT_POLARITR_3;//
	volatile int INT_ANY_3;//2E4

};

int IOInit();
//void SetIOOut();
//void ReSetIOOut();
void IOOutput(uint8_t Value);
uint16_t IOInput();

volatile struct GPIO_Reg *m_GPIO_Reg;
uintptr_t m_EMIO_Addr;
int _IoNum0_x;

#endif /* GPIO_DRIVER_H_ */
