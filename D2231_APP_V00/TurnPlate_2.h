/*
 * TurnPlate_2.h
 *
 *  Created on: 2025-12-30
 *      Author: ThinkPad
 */

#ifndef TURNPLATE_2_H_
#define TURNPLATE_2_H_

#include "Gloable_Schema.h"

enum
{
	TurnPlate_2_Mode_Motor_Add = 0xFC //×ªÅÌ1µç»ú
};


void TurnPlate_2_Module(NetCmd *cmd);
int TurnPlate_2_Move(char * _steps);

#endif /* TURNPLATE_2_H_ */
