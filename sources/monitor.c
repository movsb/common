#define IM_MONITOR
#include "monitor.h"
#include "utils.h"
#include "about.h"
#include "utils.h"
#include "msg.h"
#include "debug.h"
#include "../res/resource.h"
#include "load_driver/load_driver.h"
#include "load_driver/com_drv.h"
#include "struct/memory.h"
#include <process.h>

static char* __THIS_FILE__ = __FILE__;

/**********************************************************
文件名称:monitor.c
文件路径:../common/monitor.c
创建时间:2013-04-04,13:26:01,今天清明
文件作者:女孩不哭
文件说明:串口过滤驱动
**********************************************************/

static HFONT hFont;
static HANDLE hXorMutex;
static HANDLE hComDevice;
static MHANDLE hComHandle;
static HANDLE hEventThread;
static HANDLE hThread;
static HWND hEdit;
static HWND hDlgMain;

#define MONITOR_WINDOW_FONT_WIDTH		8
#define MONITOR_WINDOW_FONT_HEIGHT		16
#define MONITOR_MUTEX					"COM_MONITOR_MUTEX"
#define MONITOR_SERVICE_NAME			"ComMonitor"
#define MONITOR_DEVICE_NAME				"\\\\.\\SerMon"

static void add_mon_text(unsigned char* ba, int length, char* str)
{
	int len;
	char* rstr;
	static char inner_str[10240];
	debug_out(("进入add_mon_text\r\n"));
	if(ba){
		rstr=utils.hex2str(ba,&length,0,0,&inner_str[0],__ARRAY_SIZE(inner_str));
		if(!rstr){
			utils.msgerr(hDlgMain,"<add_mon_text>");
			return;
		}
	}
	len=GetWindowTextLength(hEdit);
	Edit_SetSel(hEdit,len,len);
	Edit_ReplaceSel(hEdit,str);
	if(ba){
		len=GetWindowTextLength(hEdit);
		Edit_SetSel(hEdit,len,len);
		Edit_ReplaceSel(hEdit,rstr);
		len=GetWindowTextLength(hEdit);
		Edit_SetSel(hEdit,len,len);
		Edit_ReplaceSel(hEdit,"\r\n");
	}
	if(ba && rstr!=inner_str){
		memory.free_mem((void**)&rstr,"<add_mon_text>");
	}
	return;
}

static void ProcessDataPacket(IOReq* pIrp)
{
	static char inner_str[10240];
	switch(pIrp->type)
	{
	case REQ_OPEN:add_mon_text(NULL,0,"串口已打开!\r\n");break;
	case REQ_CLOSE:add_mon_text(NULL,0,"串口已关闭!\r\n");break;
	case REQ_READ:
		{
			unsigned char* ba = (unsigned char*)pIrp+sizeof(IOReq);
			add_mon_text(ba,pIrp->SizeCopied,"读取:");
			//char str[64];
			//sprintf(str,"读取字节数:%d\r\n",pIrp->SizeCopied);
			//add_mon_text(NULL,0,str);
			//break;
			break;
		}
	case REQ_WRITE:
		{
			unsigned char* ba = (unsigned char*)pIrp+sizeof(IOReq);
			add_mon_text(ba,pIrp->SizeCopied,"写入:");
			//char str[64];
			//sprintf(str,"写入字节数:%d\r\n",pIrp->SizeCopied);
			//add_mon_text(NULL,0,str);
			break;
		}
	case REQ_SETBAUDRATE:
		{
			char ts[128];
			_snprintf(ts,__ARRAY_SIZE(ts),"设置波特率为:%u\r\n",*(unsigned long*)((unsigned char*)pIrp+sizeof(IOReq)));
			add_mon_text(NULL,0,ts);
			break;
		}
	case REQ_SETLINECONTROL:
		{
			char ts[128];
			SERIAL_LINE_CONTROL* pslc = (SERIAL_LINE_CONTROL*)((unsigned char*)pIrp+sizeof(IOReq));
			char* stopbits = pslc->StopBits==ONESTOPBIT?"1位":(pslc->StopBits==ONE5STOPBITS?"1.5位":"2位");
			char* parity;
			switch(pslc->Parity){
				case EVENPARITY:parity="偶";break;
				case ODDPARITY:parity="奇";break;
				case NOPARITY:parity="无";break;
				case MARKPARITY:parity="标记";break;
				case SPACEPARITY:parity="空格";break;
				default:parity="<未知>";break;
			}
			_snprintf(ts,__ARRAY_SIZE(ts),"停止位:%s,校验方式:%s校验,字节长:%d位\r\n",stopbits,parity,pslc->WordLength);
			add_mon_text(NULL,0,ts);
			break;
		}
	}
	memory.free_mem((void**)&pIrp,"<ProcessDataPacket>");
}

static unsigned __stdcall MonitorThreadProc(void* pv)
{
	DWORD size,dw;
	OVERLAPPED o={0};
	HANDLE hEventsArray[2];
	o.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	utils.assert_expr((void*)o.hEvent,"MonitorThreadProc");
	hEventsArray[1] = hEventThread;
	hEventsArray[0] = o.hEvent;
	debug_out(("进入MonitorThreadProc\r\n"));
	for(;;){
		BOOL WaitRet,IoRet;
		BOOL flag;
		IoRet = DeviceIoControl(hComDevice,IOCTL_COMMON_GETINFOSIZE,&hComHandle,sizeof(MHANDLE),&size,sizeof(size),&dw,&o);
		if(IoRet == TRUE){//----------请求完成

		}else{//----------------------请求未完成
			if(GetLastError()!=ERROR_IO_PENDING){
				utils.msgerr(hDlgMain,"MonitorThreadProc");
				return 0;
			}
			debug_out(("IOCTL_SERMON_GETINFOSIZE 挂起\r\n"));
			WaitRet = WaitForMultipleObjects(2,hEventsArray,FALSE,INFINITE);
			debug_out(("两者之一被激发!\r\n"));
			flag = GetOverlappedResult(hComDevice,&o,&dw,FALSE);
			if(WaitRet==WAIT_OBJECT_0+1 || !flag){//---关闭
				debug_out(("线程退出!\r\n"));
				return 0;
			}
		}
		do{
			unsigned char* pb = (unsigned char*)GET_MEM(size);
			IoRet = DeviceIoControl(hComDevice,IOCTL_COMMON_GETINFO,&hComHandle,sizeof(hComHandle),pb,size,&dw,&o);
			if(IoRet == TRUE){//----成功读取数据
				ProcessDataPacket((IOReq*)pb);
				continue;
			}else{
				if(GetLastError()!=ERROR_IO_PENDING){
					utils.msgerr(hDlgMain,"IOCTL_SERMON_GETINFO");
					memory.free_mem((void**)&pb,"MonitorThreadProc");
					return 0;
				}
				WaitRet = WaitForMultipleObjects(2,hEventsArray,FALSE,INFINITE);
				flag = GetOverlappedResult(hComDevice,&o,&dw,FALSE);
				if(WaitRet == WAIT_OBJECT_0+1 || !flag){
					memory.free_mem((void**)&pb,"<>");
					debug_out(("线程退出!\r\n"));
					return 0;
				}
				ProcessDataPacket((IOReq*)pb);
			}
		}while(0);
	}
}


#define SET_RESULT(result) SetDlgMsgResult(hWndDlg,uMsg,result)

static INT_PTR __stdcall MonitorWindowProc(HWND hWndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_SIZE:
		if(wParam==SIZE_MAXIMIZED || wParam==SIZE_RESTORED){
			int w=LOWORD(lParam);
			int h=HIWORD(lParam);
			MoveWindow(hEdit,0,0,w,h,TRUE);
			return 0;
		}
		break;
	case WM_INITDIALOG:
		{
			hDlgMain = hWndDlg;
			hEdit = GetDlgItem(hWndDlg,IDC_EDIT_MONITOR_RECV);
			SendMessage(hEdit,EM_SETLIMITTEXT,(WPARAM)(1<<22),0);
			hEventThread=CreateEvent(NULL,TRUE,FALSE,NULL);
			utils.assert_expr((void*)hEventThread,"WM_INITDIALOG");
			hThread = (HANDLE)_beginthreadex(NULL,0,MonitorThreadProc,NULL,0,NULL);
			utils.assert_expr((void*)hThread,"_beginthreadex");
			return 0;
		}
	case WM_CLOSE:
		{
			do{
				//HANDLE h = CreateFile
			}while(0);
			debug_out(("进入WM_CLOSE\r\n"));
			SetEvent(hEventThread);
			debug_out(("进入WM_CLOSE等待\r\n"));
			WaitForSingleObject(hThread,1000);
			debug_out(("线程等待结束\r\n"));
			StartMonitor(NULL,0);
			debug_out(("停止过滤\r\n"));
			CloseHandle(hEventThread);
			debug_out(("关闭hEventThread\r\n"));
			CloseHandle(hComDevice);
			debug_out(("关闭hComDevice\r\n"));
			CloseHandle(hThread);
			debug_out(("关闭hThread\r\n"));
			DoLoadDriver(0);
			debug_out(("卸载驱动\r\n"));
			ReleaseMutex(hXorMutex);
			CloseHandle(hXorMutex);
			EndDialog(hWndDlg,0);
			SET_RESULT(1);
			return 1;
		}
	}
	return 0;
}

static int DoLoadDriver(int load)
{
	if(load){
		char sys[MAX_PATH];
		GetModuleFileName(NULL,sys,__ARRAY_SIZE(sys));
		strcpy(strrchr(sys,'\\'),"\\common.sys");
		if(!driver.load(sys,MONITOR_SERVICE_NAME,"Com Monitor",1)){
			utils.msgbox(hDlgMain,MB_ICONERROR,COMMON_NAME,"无法加载驱动!");
			return 0;
		}
		hComDevice = CreateFile(MONITOR_DEVICE_NAME,GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,FILE_SHARE_WRITE|FILE_SHARE_READ,
			NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,NULL);
		if(hComDevice == INVALID_HANDLE_VALUE){
			utils.msgerr(hDlgMain,"CreateFile");
			DoLoadDriver(0);
			return 0;
		}
		return 1;
	}else{
		if(!driver.unload(MONITOR_SERVICE_NAME)){
			utils.msgbox(hDlgMain,MB_ICONERROR,COMMON_NAME,"未能成功卸载驱动程序!");
			return 0;
		}
	}
	return 1;
}


// "\\??\\COMx"
int StartMonitor(wchar_t* com_str, int start)
{
	BOOL ret;
	DWORD dw;
	if(start){
		ret=DeviceIoControl(hComDevice,IOCTL_COMMON_STARTMONITOR,com_str,(wcslen(com_str)+1)*sizeof(wchar_t),&hComHandle,sizeof(hComHandle),&dw,NULL);
		if(ret==FALSE){
			utils.msgerr(hDlgMain,"无法过滤驱动!");
			return 0;
		}
	}else{
		debug_out(("进入STOPMONITOR\r\n"));
		ret=DeviceIoControl(hComDevice,IOCTL_COMMON_STOPMONITOR,&hComHandle,sizeof(hComHandle),NULL,0,&dw,NULL);
		debug_out(("离开STOPMONITOR\r\n"));
		if(ret==FALSE){
			utils.msgbox(hDlgMain,MB_ICONERROR,COMMON_NAME,"未能成功停止!");
			return 0;
		}
	}
	return 1;
}

int ShowMonitorWindow(void)
{
	hXorMutex = CreateMutex(NULL,FALSE,MONITOR_MUTEX);
	if(GetLastError()==ERROR_ALREADY_EXISTS){
		CloseHandle(hXorMutex);
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"当前不支持过滤多个设备!");
		return 0;
	}
	if(!DoLoadDriver(1)){
		ReleaseMutex(hXorMutex);
		CloseHandle(hXorMutex);
		return 0;
	}
	if(!StartMonitor(L"\\??\\COM9",1)){
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"无法开始记录!");
		ReleaseMutex(hXorMutex);
		CloseHandle(hXorMutex);
		CloseHandle(hComDevice);
		DoLoadDriver(0);
		return 0;
	}
	DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DLG_MONITOR),NULL,MonitorWindowProc,0);
	return 1;
}

int CloseMonitorWindow(void)
{
	if(IsWindow(hDlgMain)){
		SendMessage(hDlgMain,WM_CLOSE,0,0);
		return 1;
	}
	return 1;
}
