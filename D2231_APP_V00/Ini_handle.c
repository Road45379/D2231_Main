/*
 * Ini_handle.c
 *
 *  Created on: 2025-12-26
 *      Author: ThinkPad
 */

/*
 * 查找INI文件的title和key
 * 返回值 ： 1 找到 title和key
 * 		2 找到title但未找到key
 * 		0  均未找到
 */
#include "Gloable_Schema.h"
int FindIniKey(char *title, char *key, char *filename)
{
	FILE *fp;
	int  flag = 0;
	char sTitle[32], *wTmp;
	char sLine[4096];

	sprintf(sTitle, "[%s]", title);
	if (NULL == (fp = fopen(filename, "r")))
	{
		perror("fopen");
		return -1;
	}

	while (NULL != fgets(sLine, 4096, fp))
	{
		/// 这是注释行  ///
		if (0 == strncmp("//", sLine, 2)) continue;
		if ('#' == sLine[0])              continue;

		wTmp = strchr(sLine, '=');//查找给定字符的第一个匹配之处
		if ((NULL != wTmp) && (1 == flag))
		{
			if (0 == strncmp(key, sLine, wTmp - sLine))  /// 长度依文件读取的为准  ///
			{
				sLine[strlen(sLine) - 1] = '\0';
				fclose(fp);
				return 1;
			}
		}
		else
		{
			if (0 == strncmp(sTitle, sLine, strlen(sLine) - 1))  /// 长度依文件读取的为准  ///
			{
				flag = 1; /// 找到标题位置  ///
			}
		}
	}
	fclose(fp);
	if(flag == 1)
	{
		return 2;
	}
	return 0;
}


char *GetIniKeyString(char *title, char *key, char *filename)
{
	FILE *fp;
	int  flag = 0;
	char sTitle[32], *wTmp;
	char sLine[4096];

	printf("file name : %s\r\n", filename);

	sprintf(sTitle, "[%s]", title);
	if (NULL == (fp = fopen(filename, "r")))
	{
		perror("fopen");
		return NULL;
	}

	while (NULL != fgets(sLine, 4096, fp))
	{
		/// 这是注释行  ///
		if (0 == strncmp("//", sLine, 2)) continue;
		if ('#' == sLine[0])              continue;

		wTmp = strchr(sLine, '=');//查找给定字符的第一个匹配之处
		if ((NULL != wTmp) && (1 == flag))
		{
			if (0 == strncmp(key, sLine, wTmp - sLine))  /// 长度依文件读取的为准  ///
			{
				sLine[strlen(sLine) - 1] = '\0';
				fclose(fp);
				return wTmp + 1;
			}
		}
		else
		{
			if (0 == strncmp(sTitle, sLine, strlen(sLine) - 1))  /// 长度依文件读取的为准  ///
			{
				flag = 1; /// 找到标题位置  ///
			}
		}
	}
	fclose(fp);
	return NULL;
}

int PutIniKeyString(char *title,char *key,char *val,char *filename)
{
    FILE *fpr, *fpw;
    int  flag = 0;
   // char sLine[4096], sTitle[32], *wTmp;
    char sLine[4096] = "";
    char sTitle[32] = "";
    char *wTmp;

    sprintf(sTitle, "[%s]", title);
    if (NULL == (fpr = fopen(filename, "r")))
    {
    	perror("fopen");// 读取原文件
    	fpr = NULL;
    	return -1;
    }
    sprintf(sLine, "%s.tmp", filename);
    if (NULL == (fpw = fopen(sLine,    "w")))
    {
    	perror("fopen");// 写入临时文件
    	fpw = NULL;
    	return -2;
    }
    memset(sLine, 0 , sizeof(sLine));
    while (NULL != fgets(sLine, 4096, fpr)) {
        if (2 != flag) { // 如果找到要修改的那一行，则不会执行内部的操作
            wTmp = strchr(sLine, '=');
            if ((NULL != wTmp) && (1 == flag)) {
                if (0 == strncmp(key, sLine, wTmp-sLine)) { // 长度依文件读取的为准
                    flag = 2;// 更改值，方便写入文件
                    sprintf(wTmp + 1, "%s\n", val);
                }
            } else {
                if (0 == strncmp(sTitle, sLine, strlen(sLine) - 1)) { // 长度依文件读取的为准
                    flag = 1; // 找到标题位置
                }
            }
        }
        fputs(sLine, fpw); // 写入临时文件
        memset(sLine, 0 , sizeof(sLine));
    }
    fclose(fpr);
    fclose(fpw);
    fpr = NULL;
    fpw = NULL;

    sprintf(sLine, "%s.tmp", filename);
    return rename(sLine, filename);// 将临时文件更新到原文件
}
