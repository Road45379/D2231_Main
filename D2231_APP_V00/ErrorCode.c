/*
 * ErrorCode.c
 *
 *  Created on: 2026-1-26
 *      Author: ThinkPad
 */
#include "ErrorCode.h"


Motor_Err ManipulatorModule_Motor_Err[4] = {{ManipulatorModule_X_Communication_Err, ManipulatorModule_X_Reset_Err, ManipulatorModule_X_Collision},
											{ManipulatorModule_Y_Communication_Err, ManipulatorModule_Y_Reset_Err, ManipulatorModule_Y_Collision},
											{ManipulatorModule_Z_Communication_Err, ManipulatorModule_Z_Reset_Err, ManipulatorModule_Z_Collision},
											{ManipulatorModule_HAND_Communication_Err, ManipulatorModule_HAND_Reset_Err, ManipulatorModule_HAND_Collision}};

/*
 * ´íÎóÂëºÏ³É
 */
int CompoundErrorCode(int ModuleNum, int ErrorNum)
{
	return (ModuleNum << 8) | ErrorNum;
}
