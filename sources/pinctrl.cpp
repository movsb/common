#include "StdAfx.h"
#include "pinctrl.h"
#include "../res/resource.h"

static HWND hDtr,hRts;

static char* __THIS_FILE__ = __FILE__;


static int dtr[3] = {DTR_CONTROL_DISABLE,DTR_CONTROL_ENABLE,DTR_CONTROL_HANDSHAKE};
static int rts[3] = {RTS_CONTROL_DISABLE,RTS_CONTROL_ENABLE,RTS_CONTROL_HANDSHAKE};
static char* sdtr[3] = {"(1)DTR_CONTROL_DISABLE","(0)DTR_CONTROL_ENABLE","(0)DTR_CONTROL_HANDSHAKE"};
static char* srts[3] = {"(1)RTS_CONTROL_DISABLE","(0)RTS_CONTROL_ENABLE","(0)RTS_CONTROL_HANDSHAKE"};

namespace Common{

	int c_pinctrl_dlg::axisx = -1;
	int c_pinctrl_dlg::axisy = -1;

	LRESULT c_pinctrl_dlg::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch(uMsg)
		{
		case WM_CLOSE:
			{
				RECT rc;
				GetWindowRect(m_hWnd,&rc);
				axisx=rc.left;
				axisy=rc.top;
				DestroyWindow();
				return 0;
			}
		case WM_COMMAND:
			{
				switch(LOWORD(wParam))
				{
				case IDC_PINCTRL_OK:
					if(HIWORD(wParam)!=BN_CLICKED)
						return 0;
					if(!_info->is_opened()){
						MessageBox(m_hWnd,"没有串口设备被打开!",COMMON_NAME,MB_ICONINFORMATION);
						return 0;
					}
					_config->dcb.fDtrControl = dtr[ComboBox_GetCurSel(hDtr)];
					_config->dcb.fRtsControl = rts[ComboBox_GetCurSel(hRts)];
					if(!SetCommConfig(_info->get_handle(),_config,sizeof(*_config))){
						msgerr("设置DTR/RTS时错误!");
						return 0;
					}
					EnableWindow(GetDlgItem(m_hWnd,IDC_PINCTRL_OK),FALSE);
					break;
				case IDC_CBO_PINCTRL_DTR:
				case IDC_CBO_PINCTRL_RTS:
					if(HIWORD(wParam)==CBN_SELENDOK){
						EnableWindow(GetDlgItem(m_hWnd,IDC_PINCTRL_OK),TRUE);
						return 0;
					}
					break;
				}
				return 0;
			}
		case WM_INITDIALOG:
			{
				DWORD size = sizeof(*_config);
				if(!_info->is_opened()){
					msgbox(MB_ICONEXCLAMATION,COMMON_NAME,"请先打开一个串口设备!");
					DestroyWindow();
					return 0;
				}
				if(!GetCommConfig(_info->get_handle(),_config,&size)){
					msgerr("获取串口配置时错误");
					DestroyWindow();
					return 0;
				}
				hDtr = GetDlgItem(m_hWnd,IDC_CBO_PINCTRL_DTR);
				hRts = GetDlgItem(m_hWnd,IDC_CBO_PINCTRL_RTS);
				for(;;){
					int i;
					for(i=0; i<3; i++){
						ComboBox_AddString(hDtr,sdtr[i]);
						ComboBox_AddString(hRts,srts[i]);
					}
					ComboBox_SetCurSel(hDtr,_config->dcb.fDtrControl);
					ComboBox_SetCurSel(hRts,_config->dcb.fRtsControl);
					break;
				}
				SetWindowPos(m_hWnd,0,axisx,axisy,0,0,SWP_NOSIZE|SWP_NOZORDER);
				if(axisx==-1 && axisy==-1) 
					CenterWindow();
			}
		}
		return CWnd::HandleMessage(uMsg, wParam, lParam, bHandled);
	}

}
