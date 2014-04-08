#ifndef __MSG_H__
#define __MSG_H__
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#include <Windows.h>
#include <WindowsX.h>
#include <Dbt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "struct/list.h"

#pragma warning(push)
#pragma warning(disable:4201)
struct msg_s{
	struct{
		HWND hWndMain;
		HINSTANCE hInstance;
		HANDLE hComPort;
		HWND hEditRecv;
		HWND hEditRecv2;
		HWND hEditSend;
		HFONT hFont;
		HACCEL hAccel;
		//由于用户需要拉伸窗口大小,所以程序将允许左右拉伸窗口,高度变化没啥用处
		struct{
			RECT rcWindowC;
			RECT rcWindow;
			RECT rcRecv;		
			RECT rcRecvGroup;
			RECT rcSend;
			RECT rcSendGroup;
		}WndSize;
	};

	int (*run_app)(void);
	int (*on_create)(HWND hWnd, HINSTANCE hInstance);
	int (*on_close)(void);
	int (*on_destroy)(void);
	int (*on_command)(HWND hwhWndCtrl, int id, int codeNotify);
	int (*on_device_change)(WPARAM event, DEV_BROADCAST_HDR* pDBH);
	int (*on_setting_change)(void);
	int (*on_timer)(int id);
	int (*on_size)(int width,int height);
	int (*on_sizing)(WPARAM edge,RECT* pRect);
	int (*on_app)(UINT uMsg,WPARAM wParam,LPARAM lParam);
};
#pragma warning(pop)

int init_msg(void);
enum{TIMER_ID_THREAD};

#ifndef __MSG_C__
	extern struct msg_s msg;
#else
#undef __MSG_C__
LRESULT CALLBACK RecvEditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//消息处理函数声明
int run_app(void);
int on_create(HWND hWnd, HINSTANCE hInstance);
int on_close(void);
int on_destroy(void);
int on_command(HWND hwhWndCtrl, int id, int codeNotify);
int on_activateapp(BOOL bActivate);
int on_device_change(WPARAM event, DEV_BROADCAST_HDR* pDBH);
int on_setting_change(void);
int on_timer(int id);
int on_size(int width,int height);
int on_sizing(WPARAM edge,RECT* pRect);
int on_app(UINT uMsg,WPARAM wParam,LPARAM lParam);

typedef struct{
	HWND hWnd;
	list_s entry;
}WINDOW_ITEM;

#endif

#endif//!__MSG_H__
