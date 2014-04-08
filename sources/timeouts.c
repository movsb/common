#include "timeouts.h"
#include "msg.h"
#include "utils.h"
#include "about.h"
#include "../res/resource.h"

static char* __THIS_FILE__ = __FILE__;

/**********************************************************
文件名称:timeouts.c
文件路径:./common/timeouts.c
创建时间:2013-2-27,16:44:25
文件作者:女孩不哭
文件说明:串口超时设置
**********************************************************/

extern COMMTIMEOUTS ctimeouts;
static HWND hWndTimeouts;

INT_PTR CALLBACK TimeoutsDlgProc(HWND hWndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);


INT_PTR CALLBACK TimeoutsDlgProc(HWND hWndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			char str[16];
			#pragma warning(push)
			#pragma warning(disable:4127)
			#define _SETTEXT(timeout,id) \
				do{\
					_snprintf(str,__ARRAY_SIZE(str),"%d",timeout);\
					SetDlgItemText(hWndDlg,id,str);\
				}while(0)
			_SETTEXT(ctimeouts.ReadIntervalTimeout,IDC_CTO_EDIT_READ_INTERVAL);
			_SETTEXT(ctimeouts.ReadTotalTimeoutMultiplier,IDC_CTO_EDIT_READ_MULTIPLIER);
			_SETTEXT(ctimeouts.ReadTotalTimeoutConstant,IDC_CTO_EDIT_READ_CONSTANT);
			_SETTEXT(ctimeouts.WriteTotalTimeoutMultiplier,IDC_CTO_EDIT_WRITE_MULTIPLIER);
			_SETTEXT(ctimeouts.WriteTotalTimeoutConstant,IDC_CTO_EDIT_WRITE_CONSTANT);
			#undef _SETTEXT
			#pragma warning(pop)
			EnableMenuItem(GetSystemMenu(hWndDlg,FALSE),SC_CLOSE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			utils.center_window(hWndDlg,msg.hWndMain);
			SetFocus(GetDlgItem(hWndDlg,IDC_CTO_BTN_CANCEL));
			//...
			hWndTimeouts = hWndDlg;
			return 0;
		}
	case WM_COMMAND:
		{
			if(HIWORD(wParam)!=BN_CLICKED)
				return 0;
			switch(LOWORD(wParam))
			{
			case IDC_CTO_BTN_CANCEL:
				hWndTimeouts=NULL;
				SendMessage(msg.hWndMain,WM_APP+0,1,(LPARAM)hWndTimeouts);
				DestroyWindow(hWndDlg);
				return 0;
			case IDC_CTO_BTN_OK:
				{
					int value;
					#pragma warning(push)
					#pragma warning(disable:4127)
					#define _SETVALUE(timeout,id) \
							do{\
								BOOL bTranslated;\
								value=GetDlgItemInt(hWndDlg,id,&bTranslated,FALSE);\
								if(!bTranslated){\
										MessageBox(hWndDlg,#timeout" 设置有误!",COMMON_NAME,MB_ICONEXCLAMATION);\
										return 0;\
								}else{\
									timeout=value;\
								}\
							}while(0)
					_SETVALUE(ctimeouts.ReadIntervalTimeout,IDC_CTO_EDIT_READ_INTERVAL);
					_SETVALUE(ctimeouts.ReadTotalTimeoutMultiplier,IDC_CTO_EDIT_READ_MULTIPLIER);
					_SETVALUE(ctimeouts.ReadTotalTimeoutConstant,IDC_CTO_EDIT_READ_CONSTANT);
					_SETVALUE(ctimeouts.WriteTotalTimeoutMultiplier,IDC_CTO_EDIT_WRITE_MULTIPLIER);
					_SETVALUE(ctimeouts.WriteTotalTimeoutConstant,IDC_CTO_EDIT_WRITE_CONSTANT);
					#undef _SETVALUE
					#pragma warning(pop)
					if(msg.hComPort!=INVALID_HANDLE_VALUE){
						int ret;
						ret=MessageBox(hWndDlg,
							"当前已有串口被打开,超时设置要怎样生效?\n\n"
							"[  是]   立即生效\n"
							"[  否]   下次打开串口设备时生效\n"
							"[取消]    回到超时设置",
							"超时设置",
							MB_YESNOCANCEL|MB_ICONEXCLAMATION);
						if(ret==IDYES){
							if(msg.hComPort == INVALID_HANDLE_VALUE){
								utils.msgbox(hWndDlg,MB_ICONEXCLAMATION,COMMON_NAME,
									"在等待过程打开的串口中已经被关闭, 下一次打开时才会生效!");
							}else{
								if(!SetCommTimeouts(msg.hComPort,&ctimeouts)){
									utils.msgerr(hWndDlg,"设置超时失败");
									return 0;
								}
							}
						}else if(ret==IDNO){
						}else if(ret==IDCANCEL){
							return 0;
						}
					}else{
						MessageBox(hWndDlg,"超时设置将在下次打开串口设备时生效!",COMMON_NAME,MB_ICONINFORMATION);
					}
					hWndTimeouts=NULL;
					SendMessage(msg.hWndMain,WM_APP+0,1,(LPARAM)hWndTimeouts);
					DestroyWindow(hWndDlg);
					return 0;
				}
			case IDC_CTO_BTN_DEFAULT:
				SetDlgItemText(hWndDlg,IDC_CTO_EDIT_READ_INTERVAL,"0");
				SetDlgItemText(hWndDlg,IDC_CTO_EDIT_READ_MULTIPLIER,"1");
				SetDlgItemText(hWndDlg,IDC_CTO_EDIT_READ_CONSTANT,"0");
				SetDlgItemText(hWndDlg,IDC_CTO_EDIT_WRITE_MULTIPLIER,"1");
				SetDlgItemText(hWndDlg,IDC_CTO_EDIT_WRITE_CONSTANT,"0");
				MessageBox(hWndDlg,"已还原为默认值,若要保存请点击 确定!",COMMON_NAME,MB_ICONINFORMATION);
				return 0;
			}
		}
	}
	UNREFERENCED_PARAMETER(lParam);
	return 0;
}

int ShowTimeouts(void)
{
	/*if(msg.hComPort!=INVALID_HANDLE_VALUE){
		utils.msgbox(MB_ICONEXCLAMATION,COMMON_NAME,"请先关闭已经打开的串口设备!");
		return 0;
	}*/
	if(hWndTimeouts){
		SetWindowPos(hWndTimeouts,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		return 0;
	}
	//DialogBoxParam(msg.hInstance,MAKEINTRESOURCE(IDD_DLG_TIMEOUTS),NULL,TimeoutsDlgProc,0);
	hWndTimeouts = CreateDialogParam(msg.hInstance,MAKEINTRESOURCE(IDD_DLG_TIMEOUTS),msg.hWndMain,TimeoutsDlgProc,0);
	ShowWindow(hWndTimeouts,SW_SHOW);
	SendMessage(msg.hWndMain,WM_APP+0,0,(LPARAM)hWndTimeouts);
	return 0;
}