#include <stdio.h>
#include <ctype.h>
#include <process.h>

#define __MSG_C__
#include "msg.h"
#include "utils.h"
#include "deal.h"
#include "about.h"
#include "comm.h"
#include "asctable.h"
#include "str2hex.h"
#include "monitor.h"
#include "draw.h"
#include "debug.h"
#include "struct/memory.h"
#include "struct/list.h"
#include "../res/resource.h"
#include "layout/SdkLayout.h"

void* pRoot;

struct msg_s msg;

static list_s list_head;
CRITICAL_SECTION window_critical_section;

static char* __THIS_FILE__  = __FILE__;



//消息结构体初始化, 构造函数
int init_msg(void)
{
	memset(&msg, 0, sizeof(msg));
	msg.run_app           = run_app;
	msg.on_create         = on_create;
	msg.on_close          = on_close;
	msg.on_destroy        = on_destroy;
	msg.on_command        = on_command;
	msg.on_timer          = on_timer;
	msg.on_device_change  = on_device_change;
	msg.on_setting_change = on_setting_change;
	msg.on_size           = on_size;
	msg.on_app			  = on_app;
	return 1;
}

/**************************************************
函  数:RecvEditWndProc@16
功  能:接收区EDIT的子类过程,取消鼠标选中文本时对插入的文本造成的干扰
参  数:
返回值:
说  明:由EM_SETSEL 到 EM_REPLACESEL这其间不允许选中
**************************************************/
WNDPROC OldRecvEditWndProc = NULL;
LRESULT CALLBACK RecvEditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int fEnableSelect = 1;
	switch(uMsg)
	{
	case EM_SETSEL:
		fEnableSelect = 0;
		return CallWindowProc(OldRecvEditWndProc, hWnd, uMsg, wParam, lParam);
	case EM_REPLACESEL:
		{
			LRESULT ret;
			ret = CallWindowProc(OldRecvEditWndProc, hWnd, uMsg, wParam, lParam);
			fEnableSelect = 1;
			return ret;
		}
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
		if(fEnableSelect==0 || comm.fShowDataReceived&&msg.hComPort!=INVALID_HANDLE_VALUE){
			if(uMsg==WM_LBUTTONDBLCLK || uMsg==WM_LBUTTONDOWN){
				SetFocus(hWnd);
			}
			return 0;
		}
		break;
	case WM_CONTEXTMENU:
		if(comm.fShowDataReceived&&msg.hComPort!=INVALID_HANDLE_VALUE)
			return 0;
		else 
			break;
	}
	return CallWindowProc(OldRecvEditWndProc, hWnd, uMsg, wParam, lParam);
}

WNDPROC OldRecv2EditWndProc = NULL;
LRESULT CALLBACK Recv2EditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int fEnableSelect = 1;
	switch(uMsg)
	{
	case EM_SETSEL:
		fEnableSelect = 0;
		return CallWindowProc(OldRecv2EditWndProc, hWnd, uMsg, wParam, lParam);
	case EM_REPLACESEL:
		{
			LRESULT ret;
			ret = CallWindowProc(OldRecv2EditWndProc, hWnd, uMsg, wParam, lParam);
			fEnableSelect = 1;
			return ret;
		}
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
		if(fEnableSelect==0 || comm.fShowDataReceived&&msg.hComPort!=INVALID_HANDLE_VALUE){
			if(uMsg==WM_LBUTTONDBLCLK || uMsg==WM_LBUTTONDOWN){
				SetFocus(hWnd);
			}
			return 0;
		}
		break;
	case WM_CONTEXTMENU:
		if(msg.hComPort != INVALID_HANDLE_VALUE && comm.fShowDataReceived){
			POINT pt;
			HMENU hEditMenu;
			hEditMenu = GetSubMenu(LoadMenu(msg.hInstance,MAKEINTRESOURCE(IDR_MENU_EDIT_RECV)),0);
			GetCursorPos(&pt);
			CheckMenuItem(hEditMenu,MENU_EDIT_CHINESE,MF_BYCOMMAND|(comm.fDisableChinese?MF_UNCHECKED:MF_CHECKED));
			CheckMenuItem(hEditMenu,MENU_EDIT_CONTROL_CHAR,MF_BYCOMMAND|(comm.fEnableControlChar?MF_CHECKED:MF_UNCHECKED));
			CheckMenuItem(hEditMenu,MENU_EDIT_SEND_INPUT_CHAR,MF_BYCOMMAND|(comm.fEnableCharInput?MF_CHECKED:MF_UNCHECKED));
			TrackPopupMenu(hEditMenu,TPM_LEFTALIGN,pt.x,pt.y,0,msg.hWndMain,NULL);
			return 0;
		}else{
			break;
		}
	case WM_CHAR:
		{
			if(comm.fEnableCharInput && comm.fShowDataReceived && msg.hComPort!=INVALID_HANDLE_VALUE && msg.hComPort){
				int result;
				char ch[2] = {(char)wParam,'\0'};
				char* str;
				//为了把输入字符等于接收字符显示
				//如果未能正确进入临界区说明:
				//    来自其它线程的add_text请求尚未结束,此刻不能再Enter,
				//    因为Enter如果未成功则会挂起UI线程, 导致UI线程卡死
				if(!TryEnterCriticalSection(&deal.g_add_text_cs))
					return 0;
				result = deal.send_char_data(ch[0]);
				if(result==0){
					utils.msgbox(msg.hWndMain,MB_ICONERROR,"","发送字符数据失败!");
				}
				else if(result == 1){
					if(ch[0]==' ') str = "<Space>";
					else if(ch[0]=='\b') str="<Backspace> ";//多一个空格
					else if(ch[0]=='\t') str="<Tab>";
					else if(ch[0]=='\r') str="<Enter>";
					else str = "";
					deal.add_text_critical((unsigned char*)str,strlen(str));
				}
				LeaveCriticalSection(&deal.g_add_text_cs);
				return 0;
			}
			//fall through
		}
	}
	return CallWindowProc(OldRecv2EditWndProc, hWnd, uMsg, wParam, lParam);
}

#define _SETRESULT(_msg,_result,_msgret) \
	case _msg:SetDlgMsgResult(hWnd,_msg,_result);return _msgret
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		//---
		_SETRESULT(WM_CLOSE,msg.on_close(),1);
		_SETRESULT(WM_DESTROY,msg.on_destroy(),1);
		_SETRESULT(WM_COMMAND,msg.on_command((HWND)lParam, LOWORD(wParam), HIWORD(wParam)),1);
		_SETRESULT(WM_DEVICECHANGE,msg.on_device_change(wParam,(DEV_BROADCAST_HDR*)lParam),1);
		_SETRESULT(WM_TIMER,msg.on_timer((int)wParam),1);
		_SETRESULT(WM_SETTINGCHANGE,msg.on_setting_change(),1);
		_SETRESULT(WM_SIZE,msg.on_size(LOWORD(lParam),HIWORD(lParam)),1);
		_SETRESULT(WM_APP,msg.on_app(uMsg,wParam,lParam),1);
		//---
	case WM_LBUTTONDOWN:
		SendMessage(msg.hWndMain,WM_NCLBUTTONDOWN,HTCAPTION,0);
		return 0;
	case WM_INITDIALOG:
		msg.on_create(hWnd,(HINSTANCE)GetModuleHandle(NULL));
		return FALSE;
	case WM_CTLCOLORBTN:
		{
			return (LRESULT)GetStockObject(NULL_BRUSH);
		}
	default:return 0;
	}
}
#undef _SETRESULT

//!!!一开始就不应该使用DialogBoxParam的,使用DialogBoxParam却又使用了DestroyWindow,呵呵,我错了
int run_app(void)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	//return DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_DLG_MAIN),NULL,(DLGPROC)MainWndProc,(LPARAM)hInstance);
	return (int)CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DLG_MAIN),NULL,(DLGPROC)MainWndProc);
}

int on_timer(int id)
{
	if(id==TIMER_ID_THREAD){
		debug_out(("进入on_timer\n"));
		comm.close(1);
		comm.update((int*)-1);
	}
	KillTimer(msg.hWndMain,TIMER_ID_THREAD);
	return 0;
}
int on_create(HWND hWnd, HINSTANCE hInstance)
{
	HICON hIcon = NULL;
	LOGFONT lf={0};

	//初始化句柄
	msg.hWndMain = hWnd;
	msg.hInstance = hInstance;
	msg.hEditRecv = GetDlgItem(hWnd, IDC_EDIT_RECV);
	msg.hEditRecv2 = GetDlgItem(hWnd, IDC_EDIT_RECV2);
	msg.hEditSend = GetDlgItem(hWnd, IDC_EDIT_SEND);
	msg.hComPort = INVALID_HANDLE_VALUE;


	//保存窗口默认的大小

	GetWindowRect(msg.hEditRecv,&msg.WndSize.rcRecv);
	GetWindowRect(msg.hEditSend,&msg.WndSize.rcSend);
	GetWindowRect(GetDlgItem(hWnd,IDC_STATIC_RECV),&msg.WndSize.rcRecvGroup);
	GetWindowRect(GetDlgItem(hWnd,IDC_STATIC_SEND),&msg.WndSize.rcSendGroup);
	GetWindowRect(hWnd,&msg.WndSize.rcWindow);
	GetClientRect(hWnd,&msg.WndSize.rcWindowC);

	//把窗体移动到屏幕中央
	utils.center_window(hWnd,NULL);
	//标题栏图标
	hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	SendMessage(msg.hWndMain, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	SetWindowText(hWnd,COMMON_NAME_AND_VERSION);
	SetFocus(GetDlgItem(hWnd,IDC_BTN_OPEN));
	//SetClassLong(hWnd,gcl_)

	//加载快捷键表
	msg.hAccel = LoadAccelerators(msg.hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));

		//数据显示框设置
		//等宽字体
	msg.hFont = CreateFont(
		10,5, /*Height,Width*/
		0,0, /*escapement,orientation*/
		FW_REGULAR,FALSE,FALSE,FALSE, /*weight, italic, underline, strikeout*/
		ANSI_CHARSET,OUT_DEVICE_PRECIS,CLIP_MASK, /*charset, precision, clipping*/
		DEFAULT_QUALITY, DEFAULT_PITCH, /*quality, and pitch*/
		"Courier"); /*font name*/
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
	strncpy(lf.lfFaceName, "Consolas", LF_FACESIZE);
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfHeight = -12;
	msg.hFont2 = CreateFontIndirect(&lf);
	if(!msg.hFont2)
		msg.hFont2 = msg.hFont;
	
	SendDlgItemMessage(msg.hWndMain, IDC_EDIT_RECV, WM_SETFONT, (WPARAM)msg.hFont, MAKELPARAM(TRUE, 0));
	SendDlgItemMessage(msg.hWndMain, IDC_EDIT_RECV2, WM_SETFONT, (WPARAM)msg.hFont2, MAKELPARAM(TRUE, 0));
	SendDlgItemMessage(msg.hWndMain, IDC_EDIT_SEND, WM_SETFONT, (WPARAM)msg.hFont2, MAKELPARAM(TRUE, 0));
	//文本显示区接收与发送缓冲大小
	SendDlgItemMessage(msg.hWndMain, IDC_EDIT_RECV, EM_SETLIMITTEXT, (WPARAM)COMMON_RECV_BUF_SIZE, 0);
	SendDlgItemMessage(msg.hWndMain, IDC_EDIT_SEND, EM_SETLIMITTEXT, (WPARAM)COMMON_SEND_BUF_SIZE, 0);
	SendDlgItemMessage(msg.hWndMain, IDC_EDIT_RECV2,EM_SETLIMITTEXT, (WPARAM)COMMON_RECV_BUF_SIZE, 0);
	OldRecvEditWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(msg.hWndMain, IDC_EDIT_RECV), GWL_WNDPROC, (LONG)RecvEditWndProc);
	OldRecv2EditWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(msg.hWndMain, IDC_EDIT_RECV2), GWL_WNDPROC, (LONG)Recv2EditWndProc);
	//ShowWindow(msg.hEditRecv2,FALSE);
	//TODO:
	comm.init();
	comm.update((int*)-1);
	if(ComboBox_GetCount(GetDlgItem(hWnd,IDC_CBO_CP))==0)
		deal.update_status("没有任何可用的串口!");

	memory.manage_mem(MANMEM_INITIALIZE,NULL);//内存管理必须放在所有我自己的内存分配get_mem被调用之前初始化
	list->init(&list_head);
	InitializeCriticalSection(&window_critical_section);
	deal.do_buf_send(SEND_DATA_ACTION_INIT,NULL);

	pRoot = NewLayout(hWnd, hInstance, MAKEINTRESOURCE(IDR_RCDATA1));
	SendMessage(hWnd, WM_SIZE, 0, 0);

	ShowWindow(hWnd,SW_SHOWNORMAL);

	return 0;
}

int on_close(void)
{
	if(msg.hComPort!=INVALID_HANDLE_VALUE){
		int ret;
		ret = utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION|MB_OKCANCEL, "提示",
			"当前正处于调试状态, 您确定要退出吗?");
		if(ret != IDOK)
			return 0;
		if(comm.close(0)==0)
			return 0;
	}
	deal.do_buf_send(SEND_DATA_ACTION_FREE,NULL);
	deal.do_buf_recv(NULL, 0,3);
	SendMessage(msg.hWndMain,WM_APP+0,2,0);
	memory.manage_mem(MANMEM_FREE,NULL);
	DeleteCriticalSection(&window_critical_section);
	DestroyWindow(msg.hWndMain);
	return 0;
}

int on_destroy(void)
{
	msg.hWndMain = NULL;

// 	if(msg.hFont){//删除创建的等宽字体
// 		DeleteObject(SelectObject(GetDC(GetDlgItem(msg.hWndMain, IDC_EDIT_RECV)), GetStockObject(NULL_PEN)));
// 		msg.hFont = NULL;
// 	}

	PostQuitMessage(0);
	return 0;
}

int on_command(HWND hWndCtrl, int id, int codeNotify)
{
	if(!hWndCtrl && !codeNotify){//Menu
		switch(id)
		{
		//Menu - Other
		case MENU_OTHER_HELP:about.show();break;
		case MENU_OTHER_NEWVERSION:about.update();break;
		case MENU_OTHER_ASCII:ShowAsciiTable();break;
		case MENU_OTHER_CALC:utils.show_expr();break;
		case MENU_OTHER_NOTEPAD:ShellExecute(NULL,"open","notepad",NULL,NULL,SW_SHOWNORMAL);break;
		case MENU_OTHER_DEVICEMGR:ShellExecute(NULL,"open","devmgmt.msc",NULL,NULL,SW_SHOWNORMAL);break;
		case MENU_OTHER_MONITOR:ShowMonitorWindow();break;
		case MENU_OTHER_DRAW:ShowDrawWindow();break;
		//Menu - More Settings
		case MENU_MORE_TIMEOUTS:comm.show_timeouts();break;
		case MENU_MORE_DRIVER:comm.hardware_config();break;
		case MENU_MORE_PINCTRL:comm.show_pin_ctrl();break;
		case MENU_OTHER_STR2HEX:ShowStr2Hex();break;
		//Menu - EditBox
		case MENU_EDIT_CHINESE:comm.switch_disp();break;
		case MENU_EDIT_CONTROL_CHAR:comm.switch_handle_control_char();break;
		case MENU_EDIT_SEND_INPUT_CHAR:comm.switch_send_input_char();break;
		}
		return 0;
	}
	if(!hWndCtrl && codeNotify==1)
	{
		switch(id)
		{
		case IDACC_SEND:		SendMessage(msg.hWndMain,WM_COMMAND,MAKEWPARAM(IDC_BTN_SEND,BN_CLICKED),(LPARAM)GetDlgItem(msg.hWndMain,IDC_BTN_SEND));break;
		case IDACC_OPEN:		SendMessage(msg.hWndMain,WM_COMMAND,MAKEWPARAM(IDC_BTN_OPEN,BN_CLICKED),(LPARAM)GetDlgItem(msg.hWndMain,IDC_BTN_OPEN));break;
		case IDACC_CLRCOUNTER:	SendMessage(msg.hWndMain,WM_COMMAND,MAKEWPARAM(IDC_BTN_CLR_COUNTER,BN_CLICKED),(LPARAM)GetDlgItem(msg.hWndMain,IDC_BTN_CLR_COUNTER));break;
		case IDACC_STOPDISP:	SendMessage(msg.hWndMain,WM_COMMAND,MAKEWPARAM(IDC_BTN_STOPDISP,BN_CLICKED),(LPARAM)GetDlgItem(msg.hWndMain,IDC_BTN_STOPDISP));break;
		}
		return 0;
	}

	switch(id)
	{
	case IDC_RADIO_SEND_CHAR:
	case IDC_RADIO_SEND_HEX:
	case IDC_RADIO_RECV_CHAR:
	case IDC_RADIO_RECV_HEX:
	case IDC_CHECK_IGNORE_RETURN:
	case IDC_CHECK_USE_ESCAPE_CHAR:
		comm.set_data_fmt();
		return 0;
	case IDC_BTN_STOPDISP:
		{
			if(comm.fShowDataReceived){//点击了暂停显示,进入到暂停模式
				SetWindowText(hWndCtrl, "继续显示(&D)");
				comm.fShowDataReceived = 0;
			}else{//点击了继续显示, 进入到显示模式
				SetWindowText(hWndCtrl, "暂停显示(&D)");
				comm.fShowDataReceived = 1;
			}

			if(comm.fShowDataReceived){
				if(!deal.last_show){
					//等待用户响应的过程中, 必须挂起读线程, 不然数据有冲突
					ResetEvent(deal.hEventContinueToRead);
					deal.do_check_recv_buf();
					SetEvent(deal.hEventContinueToRead);
				}
			}
			deal.last_show = comm.fShowDataReceived;
		
			deal.update_savebtn_status();
			return 0;
		}
	case IDC_BTN_COPY_RECV:
	case IDC_BTN_COPY_SEND:
		{
			char* buffer = NULL;
			int length = 0;
			if(id==IDC_BTN_COPY_RECV&&comm.fShowDataReceived&&msg.hComPort!=INVALID_HANDLE_VALUE){
				utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,COMMON_NAME,
					"在显示模式下不允许复制接收区数据!\n"
					"请点击 停止显示 切换到暂停显示模式.");
				return 0;
			}
			length = GetWindowTextLength(GetDlgItem(msg.hWndMain, id==IDC_BTN_COPY_RECV?(comm.data_fmt_recv?IDC_EDIT_RECV:IDC_EDIT_RECV2):IDC_EDIT_SEND));
			if(length == 0){
				MessageBeep(MB_ICONINFORMATION);
				//应lin0119要求, 取消复制成功的提示
				//utils.msgbox(msg.hWndMain,MB_ICONQUESTION,COMMON_NAME,"什么都没有,你想复制啥?");
				return 0;
			}
			buffer = (char*)GET_MEM(length+1);
			if(buffer==NULL) return 0;

			GetDlgItemText(msg.hWndMain, id==IDC_BTN_COPY_RECV?(comm.data_fmt_recv?IDC_EDIT_RECV:IDC_EDIT_RECV2):IDC_EDIT_SEND, buffer, length+1);
			if(!utils.set_clip_data(buffer)){
				utils.msgbox(msg.hWndMain,MB_ICONHAND, NULL, "操作失败!");
			}else{
				//应lin0119要求, 取消复制成功的提示
 				//utils.msgbox(msg.hWndMain,MB_ICONINFORMATION, COMMON_NAME, 
 				//	"已复制 %s区 数据到剪贴板!", id==IDC_BTN_COPY_RECV?"接收":"发送");
			}
			memory.free_mem((void**)&buffer,NULL);
			return 0;
		}
	case IDC_BTN_LOADFILE:
		{
			RECT rc;
			HWND h = GetDlgItem(msg.hWndMain,IDC_CBO_CP);
			GetWindowRect(h,&rc);
			comm.load_from_file();
			//2013-7-5 临时BUG修正
			//不知道为什么串口选择ComboBox总是最小化消失掉....
			SetWindowPos(h,0,0,0,rc.right-rc.left,rc.bottom-rc.top,SWP_NOMOVE|SWP_NOZORDER);
		}		
		return 0;
	case IDC_BTN_SAVEFILE:
		comm.save_to_file();
		return 0;
	case IDC_BTN_HELP:
		{
			HMENU hMenu;
			POINT pt;
			hMenu=GetSubMenu(LoadMenu(msg.hInstance,MAKEINTRESOURCE(IDR_MENU_OTHER)),0);
			GetCursorPos(&pt);
			TrackPopupMenu(hMenu,TPM_LEFTALIGN,pt.x,pt.y,0,msg.hWndMain,NULL);
			return 0;
		}
	case IDC_EDIT_RECV:
		if(codeNotify==EN_ERRSPACE || codeNotify==EN_MAXTEXT){
			int ret;
			ret=utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION|MB_YESNOCANCEL,NULL,
				"接收缓冲区满了, 是清空接收区数据还是保存,抑或是取消?\n\n"
				"若选择   是:将要求保存接收区数据,并清空接收区数据\n"
				"若选择   否:接收区数据将会被清空,数据不被保存\n"
				"若选择 取消:接收区数据将被保留,新数据将无法再显示\n\n"
				"除非接下来没有数据要接收,否则请不要点击取消!");
			if(ret==IDYES){
				if(comm.save_to_file()){
					msg.on_command(NULL,IDC_BTN_CLR_RECV,BN_CLICKED);
				}else{
					int a;
					a=utils.msgbox(msg.hWndMain,MB_ICONQUESTION|MB_YESNO,COMMON_NAME,"数据没有被保存,要继续清空接收缓冲区么?");
					if(a==IDYES){
						msg.on_command(NULL,IDC_BTN_CLR_RECV,BN_CLICKED);
					}
				}
			}else if(ret==IDNO){
				msg.on_command(NULL,IDC_BTN_CLR_RECV,BN_CLICKED);
			}else if(ret==IDCANCEL){
				//取消...
			}
		}
		return 0;
	case IDC_EDIT_SEND:
		if(codeNotify==EN_ERRSPACE || codeNotify==EN_MAXTEXT){
			utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION|MB_YESNO, NULL, "发送缓冲区满!");
		}else if(codeNotify == EN_CHANGE){
			if(comm.fAutoSend){
				deal.cancel_auto_send(0);
				utils.msgbox(msg.hWndMain,MB_ICONINFORMATION,COMMON_NAME,"由于发送内容已改变, 自动重发已取消!");
			}
		}
		return 0;
	case IDC_BTN_SEND:
		deal.do_send(0);
		return 0;
	case IDC_BTN_CLR_COUNTER:
		//未发送计数不需要清零
		InterlockedExchange((long volatile*)&comm.cchSent, 0);
		InterlockedExchange((long volatile*)&comm.cchReceived, 0);
		deal.update_status(NULL);
		return 0;
	case IDC_BTN_CLR_SEND:
		deal.cancel_auto_send(0);
		SetDlgItemText(msg.hWndMain,IDC_EDIT_SEND,"");
		return 0;
	case IDC_BTN_CLR_RECV:
		{
			//我觉得同时清除16进制数据和字符数据是有必要的, 不管了, 提醒下再说~
			int r = utils.msgbox(msg.hWndMain, MB_ICONQUESTION|MB_YESNOCANCEL, 
				"提示",
				"您选择了清除接收区的数据!\n\n"
				"是\t仅清除 %s 数据\n"
				"否\t清除十六进制数据和字符数据\n"
				"取消\t取消本次操作",
				comm.data_fmt_recv?"十六进制":"字符"
				);
			switch(r)
			{
			case IDCANCEL:
				return 0;
				break;
			case IDNO:
				SetDlgItemText(msg.hWndMain,IDC_EDIT_RECV,"");
				SetDlgItemText(msg.hWndMain,IDC_EDIT_RECV2,"");
				break;
			case IDYES:
				SetDlgItemText(msg.hWndMain,comm.data_fmt_recv?IDC_EDIT_RECV:IDC_EDIT_RECV2,"");
				break;
			}
			//
			if(comm.data_fmt_recv) InterlockedExchange((long volatile*)&comm.data_count,0);
			return 0;
		}
	case IDC_CHK_TOP:
		{
			int flag = IsDlgButtonChecked(msg.hWndMain, IDC_CHK_TOP);
			SetWindowPos(msg.hWndMain,flag?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			BringWindowToTop(msg.hWndMain);
			return 0;
		}
	case IDC_CHECK_SIMPLE:
		{
			int flag = !IsDlgButtonChecked(msg.hWndMain, IDC_CHECK_SIMPLE);
			VisibaleLayout(pRoot,"recv_btns",flag);
			VisibaleLayout(pRoot,"send_wnd",flag);
			VisibaleLayout(pRoot,"send_btns",flag);
			VisibaleLayout(pRoot,"auto_send",flag);
			VisibaleLayout(pRoot,"send_fmt",flag);
			//VisibaleLayout(pRoot,"recv_group",flag);
			SendMessage(msg.hWndMain, WM_SIZE, 0, 0);
			return 0;
		}
	case IDC_CHK_AUTO_SEND:
		deal.check_auto_send();
		return 0;
	case IDC_BTN_OPEN:
		{
			if(comm.fCommOpened){
				if(comm.close(0)){
					comm.update((int*)-1);
				}
			}else{
				comm.open();
			}
			deal.update_savebtn_status();
			return 0;
		}
	case IDC_BTN_MORE_SETTINGS:
		{
			POINT pt;
			HMENU hMenu;
			GetCursorPos(&pt);
			hMenu=GetSubMenu(LoadMenu(msg.hInstance,MAKEINTRESOURCE(IDR_MENU_MORE)),0);
			TrackPopupMenu(hMenu,TPM_LEFTALIGN,pt.x,pt.y,0,msg.hWndMain,NULL);
			return 0;
		}
	case IDC_CBO_CP:
		{
			static RECT rc;
			static char is_there_any_item;
			if(codeNotify == CBN_DROPDOWN){
				GetWindowRect(hWndCtrl,&rc);
				ShowWindow(msg.hEditRecv,FALSE);
				ShowWindow(msg.hEditRecv2,FALSE);
				ShowWindow(GetDlgItem(msg.hWndMain,IDC_STATIC_RECV),FALSE);
				SetWindowPos(hWndCtrl,0,0,0,rc.right-rc.left+300,rc.bottom-rc.top,SWP_NOMOVE|SWP_NOZORDER);
				if(ComboBox_GetCount(hWndCtrl)==0){
					ComboBox_AddString(hWndCtrl,"< 没 有 找 到 任 何 可 用 的 串 口 ! >  点 击 刷 新 列 表");
					is_there_any_item = 0;
				}else{
					is_there_any_item = 1;
				}
				//utils.msgbox(0,NULL,(char*)comm.update((int*)(16+ComboBox_GetCurSel(hWndCtrl))));
				return 0;
			}else if(codeNotify == CBN_SELENDOK || codeNotify==CBN_SELENDCANCEL){
				SetWindowPos(hWndCtrl,0,0,0,rc.right-rc.left,rc.bottom-rc.top,SWP_NOMOVE|SWP_NOZORDER);
				ShowWindow(comm.data_fmt_recv?msg.hEditRecv:msg.hEditRecv2,TRUE);
				ShowWindow(GetDlgItem(msg.hWndMain,IDC_STATIC_RECV),TRUE);
				if(is_there_any_item==0){
					ComboBox_ResetContent(hWndCtrl);
					comm.update((int*)-1);
				}
				return 0;
			}
			break;
		}
	case IDC_CBO_BR:
		{
			if(codeNotify == CBN_SELCHANGE){
				int count=ComboBox_GetCount(hWndCtrl);
				if(ComboBox_GetCurSel(hWndCtrl)==count-1){
					int value;
					extern BOOL GetNewBR(HWND hParent,int* value);

					if(GetNewBR(msg.hWndMain,&value))
					{
						char s[128];
						sprintf(s,"%d",value);
						ComboBox_InsertString(hWndCtrl,count-1,s);
						ComboBox_SetCurSel(hWndCtrl,count-1);
					}else{
						ComboBox_SetCurSel(hWndCtrl,0);
					}
					return 0;
				}
			}
			return 0;
		}
	}
	return 0;
}
/**************************************************
函  数:on_device_change@8
功  能:在设备发生改变检测串口设备的改动
参  数:	event - 设备事件
		pDBH - DEV_BROADCAST_HDR*
返回值:见MSDN
说  明:有没有看到大量重复的代码, 有木有!!!
**************************************************/
#if _MSC_VER > 1200			//compatible with vc6.0
#define  strnicmp _strnicmp
#endif
int on_device_change(WPARAM event, DEV_BROADCAST_HDR* pDBH)
{
	if(msg.hComPort==INVALID_HANDLE_VALUE){
		if(event==DBT_DEVICEARRIVAL){
			if(pDBH->dbch_devicetype == DBT_DEVTYP_PORT){
				DEV_BROADCAST_PORT* pPort = (DEV_BROADCAST_PORT*)pDBH;
				char* name = &pPort->dbcp_name[0];
				if(strnicmp("COM",name,3)==0){
					int com_id;
					char buff[32];
					extern HWND hComPort;
					_snprintf(buff,sizeof(buff),"已检测到串口设备 %s 的插入!",name);
					deal.update_status(buff);
					com_id = atoi(name+3);
					if(comm.update((int*)&com_id))
						ComboBox_SetCurSel(hComPort,com_id);
					SetFocus(GetDlgItem(msg.hWndMain, IDC_EDIT_SEND));
				}
			}
		}else if(event==DBT_DEVICEREMOVECOMPLETE){
			if(pDBH->dbch_devicetype==DBT_DEVTYP_PORT){
				DEV_BROADCAST_PORT* pPort=(DEV_BROADCAST_PORT*)pDBH;
				char* name = &pPort->dbcp_name[0];
				if(strnicmp("COM",name,3)==0){
					char buff[32];
					extern HWND hComPort;
					_snprintf(buff,sizeof(buff),"串口设备 %s 已移除!",name);
					deal.update_status(buff);
					comm.update((int*)-1);
					if(ComboBox_GetCount(hComPort))
						ComboBox_SetCurSel(hComPort,0);
				}
			}
		}
	}
	return TRUE;
}

int on_setting_change(void)
{
	if(msg.hComPort == INVALID_HANDLE_VALUE){
		extern HWND hComPort;
		comm.update((int*)-1);
		if(ComboBox_GetCount(hComPort)){
			ComboBox_SetCurSel(hComPort,0);
		}
	}
	return 0;
}

int on_size(int width,int height)
{
	RECT rc;
	SIZE sz;
	GetClientRect(msg.hWndMain, &rc);
	sz.cx = rc.right-rc.left;
	sz.cy = rc.bottom-rc.top;
	SizeLayout(pRoot, &sz);
	InvalidateRect(msg.hWndMain, &rc, FALSE);
	return 0;
}

int on_app(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg-WM_APP)
	{
	case 0:
		EnterCriticalSection(&window_critical_section);
		if(wParam==0){
			WINDOW_ITEM* pitem = (WINDOW_ITEM*)GET_MEM(sizeof(WINDOW_ITEM));
			pitem->hWnd = (HWND)lParam;
			list->insert_tail(&list_head,&pitem->entry);
		}else if(wParam==1){
			list_s* p = NULL;
			for(p=list_head.next; p!=&list_head; p=p->next){
				WINDOW_ITEM* pitem =  list_data(p,WINDOW_ITEM,entry);
				if(pitem->hWnd == (HWND)lParam){
					list->remove(&list_head,p);
					memory.free_mem((void**)&pitem,NULL);
					break;
				}
			}
		}else if(wParam==2){
			while(!list->is_empty(&list_head)){
				WINDOW_ITEM* pitem = list_data(list->remove_head(&list_head),WINDOW_ITEM,entry);
				DestroyWindow(pitem->hWnd);
				memory.free_mem((void**)&pitem,NULL);
			}
		}
		LeaveCriticalSection(&window_critical_section);
		return 0;
	}
	return 0;
}