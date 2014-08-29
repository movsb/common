#include <stdio.h>
#include <windows.h>

#include "debug.h"

/*
void DebugOut(char* fmt, ...)
{	
	char buf[1024] = {0};
	va_list va;
	COPYDATASTRUCT cds = {0};
	int len;

	HWND hWndDebugWindow = NULL;
	hWndDebugWindow = FindWindow("nbsg_debug_window_class", NULL);
	if(!hWndDebugWindow || !IsWindow(hWndDebugWindow))
		return;

	va_start(va, fmt);
	//vsprintf(buf, fmt, va);
	len=_vsnprintf(buf,sizeof(buf),fmt,va);
	
	cds.dwData = 0;
	cds.lpData = (void*)buf;
	cds.cbData = len+1;
	SendMessage(hWndDebugWindow, WM_COPYDATA, 0, (LPARAM)&cds);
	return;
}*/


void DebugOut(char* fmt, ...)
{	
	char buf[1024] = {0};
	va_list va;
	COPYDATASTRUCT cds = {0};
	int len,dw;
	va_start(va, fmt);
	len=_vsnprintf(buf,sizeof(buf),fmt,va);
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),buf,len,(LPDWORD)&dw,NULL);
	return;
}
