/*
 * Set_Time.h
 *
 *  Created on: 2022-3-24
 *      Author: ThinkPad
 */

#ifndef SET_TIME_H_
#define SET_TIME_H_

#include <time.h>

int SetSysTime(char *buf,char* format);
int ReadSysTime(char *buf,char* format);
struct tm *ReadSysTime_returnTm();
void get_format_time_ms(char *str_time);

#endif /* SET_TIME_H_ */
