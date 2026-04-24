/*
 * Ini_handle.h
 *
 *  Created on: 2025-12-26
 *      Author: ThinkPad
 */

#ifndef INI_HANDLE_H_
#define INI_HANDLE_H_

int FindIniKey(char *title, char *key, char *filename);
char *GetIniKeyString(char *title, char *key, char *filename);
int PutIniKeyString(char *title,char *key,char *val,char *filename);

#endif /* INI_HANDLE_H_ */
