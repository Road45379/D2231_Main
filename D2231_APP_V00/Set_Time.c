/*
 * Set_Time.c
 *
 *  Created on: 2022-3-24
 *      Author: ThinkPad
 */

#include "Set_Time.h"

#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

/*
%a 星期几的简写形式
%A 星期几的全称
%b 月份的简写形式
%B 月份的全称
%c 日期和时间
%d 月份中的日期,0-31
%H 小时,00-23
%I 12进制小时钟点,01-12
%j 年份中的日期,001-366
%m 年份中的月份,01-12
%M 分,00-59
%p 上午或下午
%S 秒,00-60
%u 星期几,1-7
%w 星期几,0-6
%x 当地格式的日期
%X 当地格式的时间
%y 年份中的最后两位数,00-99
%Y 年
%Z 地理时区名称
char *buf:表示时间的字符串
char* format：该字符串的格式
 */
int SetSysTime(char *buf,char* format)
{
    struct tm * stime = (struct tm *)malloc(sizeof(struct tm) * 1);
    struct timeval tv;
    time_t timeq;
    int rec;

    strptime( buf, format, stime );

    timeq = mktime(stime);//将时间结构数据转换成经过的秒数
    tv.tv_sec = (long)timeq;
    tv.tv_usec = 0;
    rec = settimeofday(&tv,NULL);
    free(stime);
    stime = NULL;
    if(rec <0 )
    {
        printf("settimeofday failed!\n");
        return -1;
    }
    else
    {
        printf("Set system time ok!\n");
        return 0;
    }
}

int ReadSysTime(char *buf,char* format)
{
	struct tm * stime;
	time_t timep;
	time(&timep);//获取时间
	stime = localtime(&timep);//参数timep所指的time_t结构中的信息转换成真实世界所使用的时间日期表示方法，然后将结果由结构tm返回
	strftime(buf, 64, format, stime);
	return 0;
}

struct tm * ReadSysTime_returnTm()
{
	struct tm * stime;
	time_t timep;
	time(&timep);//获取时间
	stime = localtime(&timep);//参数timep所指的time_t结构中的信息转换成真实世界所使用的时间日期表示方法，然后将结果由结构tm返回
	return stime;
}

void get_format_time_ms(char *str_time) {
    struct tm *tm_t;
    struct timeval time;

    gettimeofday(&time,NULL);
    tm_t = localtime(&time.tv_sec);
    if(NULL != tm_t) {
        sprintf(str_time,"%04d-%02d-%02d %02d:%02d:%02d.%03ld",
            tm_t->tm_year+1900,
            tm_t->tm_mon+1,
            tm_t->tm_mday,
            tm_t->tm_hour,
            tm_t->tm_min,
            tm_t->tm_sec,
            time.tv_usec/1000);
    }
}
