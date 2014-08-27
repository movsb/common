#ifndef __COMM_H__
#define __COMM_H__
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4201)
#include <SetupAPI.h>
#include <devguid.h>
#pragma warning(pop)
#include <stdio.h>

void init_comm(void);

enum DATA_FORMAT{DATA_FMT_HEX,DATA_FMT_CHAR};

#pragma warning(push)
#pragma warning(disable:4201)
struct comm_s{
	void (*init)(void);
	int (*update)(int* which);
	int (*open)(void);
	int (*close)(int reason);
	int (*save_to_file)(void);
	int (*load_from_file)(void);
	void (*set_data_fmt)(void);
	int (*hardware_config)(void);
	int (*show_pin_ctrl)(void);
	int (*show_timeouts)(void);
	int (*update_config)(int only_update);
	int (*switch_disp)(void);
	int (*switch_send_input_char)(void);
	//数据成员
	struct{
		//发送/接收数据格式
		enum DATA_FORMAT data_fmt_send;
		enum DATA_FORMAT data_fmt_recv;
		//忽略回车/字符转义
		int data_fmt_ignore_return;
		int data_fmt_use_escape_char;
		//发送/接收计数
		unsigned int cchSent;		//初始化0
		unsigned int cchReceived;	//初始化0
		unsigned int cchNotSend;	//等待发送的数据量
		//是否自动发送
		int fAutoSend;				//初始化否
		//是否显示接收内容
		int fShowDataReceived;		//初始化是
		//串口是否已经打开
		int fCommOpened;
		//是否允许显示中文
		int fDisableChinese;
		int fEnableCharInput;		//允许发送输入字符


		
		DWORD data_count;


	};
};
#pragma warning(pop)

#define COMMON_MAX_LOAD_SIZE			((unsigned long)1<<20)
#define COMMON_LINE_CCH_SEND			16
#define COMMON_LINE_CCH_RECV			16
#define COMMON_SEND_BUF_SIZE			COMMON_MAX_LOAD_SIZE
#define COMMON_RECV_BUF_SIZE			0 // un-limited //(((unsigned long)1<<20)*10)
#define COMMON_INTERNAL_RECV_BUF_SIZE	((unsigned long)1<<20)
#define COMMON_READ_BUFFER_SIZE			((unsigned long)1<<20)

#ifndef __COMM_C__
	extern struct comm_s comm;
#endif
//////////////////////////////////////////////////////////////////////////
#ifdef __COMM_C__
void init(void);
int open(void);
int get_comm_list(int* which);
int save_to_file(void);
int load_from_file(void);
void set_data_fmt(void);
int hardware_config(void);
int update_config(int only_update);
int close(int reason);
int switch_disp(void);
int switch_send_input_char(void);
#endif

#endif//!__COMM_H__
