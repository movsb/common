#ifndef __DEAL_H__
#define __DEAL_H__
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_deal(void);

/***********************************************************************
名称:用来保存待发送的数据
描述:
参数:data_size,当前数据包的大小
返回:
说明:
***********************************************************************/
enum{SEND_DATA_TYPE_NOTUSED,SEND_DATA_TYPE_USED,SEND_DATA_TYPE_MUSTFREE,SEND_DATA_TYPE_AUTO_USED,SEND_DATA_TYPE_AUTO_MUSTFREE};
enum{SEND_DATA_ACTION_GET,SEND_DATA_ACTION_RETURN,SEND_DATA_ACTION_INIT,SEND_DATA_ACTION_FREE,SEND_DATA_ACTION_RESET};
typedef struct _SEND_DATA{
	DWORD cb;						//当前数据包的大小
	DWORD data_size;				//当前包含的待发送的数据的大小
	int flag;						//对应上面的enum
	unsigned char data[10240];		//2013-03-23:增加到10KB
}SEND_DATA;

struct deal_s{
	void (*do_check_recv_buf)(void);
	int (*do_buf_send)(int action,void* pv);
	int (*do_buf_recv)(unsigned char* chs, int cb, int action);
	void (*update_status)(char* str);
	void (*update_savebtn_status)(void);
	void (*cancel_auto_send)(int reason);
	void (*check_auto_send)(void);
	unsigned int (__stdcall* thread_read)(void* pv);
	unsigned int (__stdcall* thread_write)(void* pv);
	void* (*do_send)(int action);
	int (*send_char_data)(char ch);
	void (*add_send_packet)(SEND_DATA* psd);
	SEND_DATA* (*make_send_data)(int fmt,void* data,size_t size);
	void (*start_timer)(int start);
	void (*add_text)(unsigned char* ba, int cb);
	void (*add_text_critical)(unsigned char* ba, int cb);
	//....
	int last_show;
	//计时器
	unsigned int conuter;
	struct{
		//读写管道
		HANDLE hPipeRead;
		HANDLE hPipeWrite;
		//读写线程
		HANDLE hThreadRead;
		HANDLE hThreadWrite;
		//同步事件 - since the serial driver does not support Async. I/O, deprecated
		//HANDLE hEventRead;
		//HANDLE hEventWrite;
	}thread;
	void* autoptr;//自动发送时使用的数据指针,由do_send返回
	unsigned int timer_id;	//自动发送时的定时器ID
	CRITICAL_SECTION critical_section;
	CRITICAL_SECTION g_add_text_cs;

	//在提醒用户有未显示的数据时, 必须挂起read线程
	HANDLE hEventContinueToRead;

	// 就算当前接收数据包开始是\r\n(或其它), 也并不能说明这就是一个回车换行, 因为上一包中可能是以\r/\n结尾呢!!!
	// 是不是我还不清楚怎么做才能做到所谓的"字符设备"的标准做法呢?
	// 再加上Windows的记事本"很难"取得当前最后几个字符是什么, 所以在这里保存一下最后接收的几个字符
#define DEAL_CACHE_SIZE 10240
	struct{
		int cachelen;	//转换之前的crlf长度
		int crlflen;	//转换之后的crlf长度, 按对数计
		unsigned char cache[DEAL_CACHE_SIZE];
		unsigned char* ptr; // 目前尚未使用
	}cache;
};

#ifndef __DEAL_C__
extern struct deal_s deal;
#endif

#ifdef __DEAL_C__
#undef __DEAL_C__
int do_buf_send(int action,void* pv);
int do_buf_recv(unsigned char* chs, int cb, int action);
void do_check_recv_buf(void);

void update_status(char* str);
void update_savebtn_status(void);
void cancel_auto_send(int reason);
void check_auto_send(void);


unsigned int __stdcall thread_read(void* pv);
unsigned int __stdcall thread_write(void* pv);

void* do_send(int action);
int send_char_data(char ch);

void add_send_packet(SEND_DATA* psd);
SEND_DATA* make_send_data(int fmt,void* data,size_t size);

//void add_ch(unsigned char ch);
void add_text(unsigned char* ba, int cb);
void add_text_critical(unsigned char* ba, int cb);

void start_timer(int start);

#endif


#ifdef __cplusplus
}
#endif


#endif//__DEAL_H__
