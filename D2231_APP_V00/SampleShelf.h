/*
 * SampleShelf.h
 *
 *  Created on: 2025-12-31
 *      Author: ThinkPad
 */

#ifndef SAMPLESHELF_H_
#define SAMPLESHELF_H_

#include "Gloable_Schema.h"

enum
{
	SampleShelf_Mode_Add = 0x03,//电磁铁板
	ControlBoard_1 = 0x08,//分控1
	ControlBoard_2 ,//分控2
	ControlBoard_3,//分控3
	ControlBoard_4//分控4
};

void SampleShelf_ReadState(NetCmd *cmd);
void SampleShelfModule(NetCmd *cmd);


typedef struct
{
	int Microswitch_state;
	int LED_state;
	int KEY_state;
}MagentBoard_STATE;

MagentBoard_STATE _MagentBoard_STATE;
MagentBoard_STATE _MagentBoard_STATE_tmp;

#endif /* SAMPLESHELF_H_ */
