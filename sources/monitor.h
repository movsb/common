#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <windows.h>
#include <windowsx.h>
#include "load_driver/com_drv.h"

int ShowMonitorWindow(void);
int CloseMonitorWindow(void);

#ifdef IM_MONITOR
#undef IM_MONITOR
static int DoLoadDriver(int load);
static unsigned __stdcall MonitorThreadProc(void* pv);
static INT_PTR __stdcall MonitorWindowProc(HWND hWndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
static void add_mon_text(unsigned char* ba, int length, char* str);
static void ProcessDataPacket(IOReq* pIrp);
static int StartMonitor(wchar_t* com_str, int start);
#endif
#endif//!__MONITOR_H__
