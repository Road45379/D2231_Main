/*
 * GPIO_Driver.c
 *
 *  Created on: 2019-11-28
 *      Author: ThinkPad
 */

#include "GPIO_Driver.h"

int IOInit()
{
	m_EMIO_Addr = mmap_device_io(1024, 0xE000A000);
	if (m_EMIO_Addr == MAP_DEVICE_FAILED){
		return -1;
	}
	m_GPIO_Reg = (struct GPIO_Reg *) m_EMIO_Addr;
	m_GPIO_Reg->MASK_DATA_2_LSW &= (~(0xC0));
	m_GPIO_Reg->DIRM_2 |= 0xC0;//0x7<<3;
	//m_GPIO_Reg->DIRM_2 = (m_GPIO_Reg->DIRM_2&0xFFFFFFF8);
	m_GPIO_Reg->OEN_2 |= 0xC0;//0x7<<3;
	//m_GPIO_Reg->OEN_2 =(m_GPIO_Reg->OEN_2&0xFFFFFFF8);
	return 0;
}

/*ÉèÖÃLEDµÆ*/
void IOOutput(uint8_t Value)
{
	m_GPIO_Reg->DATA_2 =(m_GPIO_Reg->DATA_2&0xFFFFFF3F)|((Value&0x3) << 6);
}

/*¶Á¿ª¸Ç´«¸ÐÆ÷×´Ì¬*/
uint16_t IOInput()
{
	return (uint16_t)((m_GPIO_Reg->DATA_2_RO >> 11) & 0x3FF);
}

/*void SetIOOut()
{
	if (m_EMIO_Addr == MAP_DEVICE_FAILED) {
		return;
	}
	m_GPIO_Reg->DATA_2 |= 0x1<<_IoNum0_x;
}

void ReSetIOOut()
{
	if (m_EMIO_Addr == MAP_DEVICE_FAILED) {
		return;
	}
	m_GPIO_Reg->DATA_2 =m_GPIO_Reg->DATA_2 & (~(0x1<<_IoNum0_x));
}*/

