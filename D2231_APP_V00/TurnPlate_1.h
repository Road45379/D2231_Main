/*
 * TurnPlate_1.h
 *
 *  Created on: 2025-12-29
 *      Author: ThinkPad
 */

#ifndef TURNPLATE_1_H_
#define TURNPLATE_1_H_

#include "Gloable_Schema.h"

enum
{
	TurnPlate_1_Mode_Motor_Add = 1, //转盘1电机
	Rotate_TeatTube_Motor_Add = 2, //旋转试管电机

};


void TurnPlate_1_Module(NetCmd *cmd);

#endif /* TURNPLATE_1_H_ */
