/*
 * TurnPlate_3.h
 *
 *  Created on: 2025-12-30
 *      Author: ThinkPad
 */

#ifndef TURNPLATE_3_H_
#define TURNPLATE_3_H_

#include "Gloable_Schema.h"

enum
{
	TurnPlate_3_Mode_Motor_Add = 0xFD //×ªÅÌ1µç»ú
};


void TurnPlate_3_Module(NetCmd *cmd);
int TurnPlate_3_Move(char * _steps);


#endif /* TURNPLATE_3_H_ */
