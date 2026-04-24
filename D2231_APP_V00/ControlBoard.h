/*
 * ControlBoard.h
 *
 *  Created on: 2026-1-8
 *      Author: ThinkPad
 */

#ifndef CONTROLBOARD_H_
#define CONTROLBOARD_H_
#include "Gloable_Schema.h"

void Get_ControlBoard_State();
void ControlBoard_1_Module(NetCmd *cmd);
int ControlBoard_Set_Mode(int ControlBoardAddr, int mode);
char ControlBoard_Turntable_Respond(int ControlBoardAddr, int carNum);

#endif /* CONTROLBOARD_H_ */
