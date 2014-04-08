#ifndef __LOADDRIVER_H__
#define __LOADDRIVER_H__

/**********************************************************
文件名称:LoadDriver.h/LoadDriver.c
文件路径:./LoadDriver/LoadDriver.h,LoadDriver.c
创建时间:2013-2-1,22:24:09
文件作者:女孩不哭
文件说明:该C语言程序实现对内核驱动程序的加载与卸载
	LoadDriver - 加载驱动
	UnloadDriver - 卸载驱动
**********************************************************/

void init_driver(void);
struct driver_s{
	int (*load)(char* DriverAbsPath, char* ServiceName, char* DisplayName,int PromptIfExists);
	int (*unload)(char* ServiceName);
};



#ifndef LOAD_DRIVER
extern struct driver_s driver;
#else
struct driver_s driver;

int LoadDriver(char* DriverAbsPath, char* ServiceName, char* DisplayName,int PromptIfExists);
int UnloadDriver(char* ServiceName);

#endif

#endif//!__LOADDRIVER_H__
