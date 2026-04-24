/*
 * TurnPlate_3_OP.h
 *
 *  Created on: 2026-1-6
 *      Author: Administrator
 */

#ifndef TURNPLATE_3_OP_H_
#define TURNPLATE_3_OP_H_

#include "Gloable_Schema.h"
#include "TurnPlate_2.h"
#include "TurnPlate_3.h"
#include "RFID.h"
#include "TurnPlate_2_OP.h"

#define RFID_4_ADDR		0x4F
#define TRACK_OFFSET_OFFSET 0x04

char IsTurnplateRelease();

void Turntable_3_Sensor_5(int val);
void Turntable_3_Sensor_6(int val);

extern int Turntable_3_trunNum;
extern int Turntable_3_lastLocatin;

#endif /* TURNPLATE_3_OP_H_ */
