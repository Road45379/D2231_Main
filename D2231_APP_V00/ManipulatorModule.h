/*
 * ManipulatorModule.h
 *机械手模块 02
 *  Created on: 2025-12-24
 *      Author: ThinkPad
 */

#ifndef MANIPULATORMODULE_H_
#define MANIPULATORMODULE_H_

#include "Gloable_Schema.h"

enum
{
	Manipulator_Mode_Motor_Add_x = 1,
	Manipulator_Mode_Motor_Add_y,
	Manipulator_Mode_Motor_Add_z,
	Manipulator_Mode_Motor_Add_hand   //电爪
};

void ManipulatorModule(NetCmd *cmd);

#endif /* MANIPULATORMODULE_H_ */
