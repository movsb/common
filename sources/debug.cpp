#include <stdio.h>
#include <windows.h>
#include <fstream>

#include "debug.h"

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

