#ifndef __PARSE_CMD__
#define __PARSE_CMD__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum{CMDI_METHOD_CHAR=0,CMDI_METHOD_HEX,};

#define CMDI_MAX_METHOD		2
#define CMDI_MAX_NAME		64
#define CMDI_MAX_COMMAND	128
#define CMDI_MAX_LINES		128

typedef struct _command_item{
	int valid;								//数据是否有效
	int bytes;								//数据最终的字节数
	char method[CMDI_MAX_METHOD];			//方式:字符/16进制
	char  name[CMDI_MAX_NAME];				//命令名
	char command[CMDI_MAX_COMMAND];			//命令原始数据
	unsigned char data[CMDI_MAX_COMMAND];	//最终数据
}command_item;

int parse_cmd(char* file,command_item** ppci,size_t* pnItems);
int make_command_item(command_item* pci);

#ifdef __cplusplus
}
#endif

#endif//!__PARSE_CMD__
