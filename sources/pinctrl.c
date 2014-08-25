#include "pinctrl.h"
#include "msg.h"
#include "utils.h"
#include "about.h"
#include "../res/resource.h"

static HWND hWndPinCtrl;
static HWND hDtr,hRts;
static int axisx,axisy;
extern COMMCONFIG cconfig;

static char* __THIS_FILE__ = __FILE__;


static int dtr[3] = {DTR_CONTROL_DISABLE,DTR_CONTROL_ENABLE,DTR_CONTROL_HANDSHAKE};
static int rts[3] = {RTS_CONTROL_DISABLE,RTS_CONTROL_ENABLE,RTS_CONTROL_HANDSHAKE};
static char* sdtr[3] = {"(1)DTR_CONTROL_DISABLE","(0)DTR_CONTROL_ENABLE","(0)DTR_CONTROL_HANDSHAKE"};
static char* srts[3] = {"(1)RTS_CONTROL_DISABLE","(0)RTS_CONTROL_ENABLE","(0)RTS_CONTROL_HANDSHAKE"};

INT_PTR CALLBACK PinCtrlDlgProc(HWND hWndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CLOSE:
		{
			RECT rc;
			GetWindowRect(hWndDlg,&rc);
			axisx=rc.left;
			axisy=rc.top;
			hWndPinCtrl=NULL;
			SendMessage(msg.hWndMain,WM_APP+0,1,(LPARAM)hWndDlg);
			DestroyWindow(hWndDlg);
			return 0;
		}
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDC_PINCTRL_OK:
				if(HIWORD(wParam)!=BN_CLICKED)
					return 0;
				if(msg.hComPort==NULL){
					MessageBox(hWndDlg,"没有串口设备被打开!",COMMON_NAME,MB_ICONINFORMATION);
					return 0;
				}
				cconfig.dcb.fDtrControl = dtr[ComboBox_GetCurSel(hDtr)];
				cconfig.dcb.fRtsControl = rts[ComboBox_GetCurSel(hRts)];
				if(!SetCommConfig(msg.hComPort,&cconfig,sizeof(cconfig))){
					utils.msgerr(hWndDlg,"设置DTR/RTS时错误!");
					return 0;
				}
				EnableWindow(GetDlgItem(hWndDlg,IDC_PINCTRL_OK),FALSE);
				break;
			case IDC_CBO_PINCTRL_DTR:
			case IDC_CBO_PINCTRL_RTS:
				if(HIWORD(wParam)==CBN_SELENDOK){
					EnableWindow(GetDlgItem(hWndDlg,IDC_PINCTRL_OK),TRUE);
					return 0;
				}
				break;
			}
			return 0;
		}
	case WM_INITDIALOG:
		{
			DWORD size = sizeof(cconfig);
			if(msg.hComPort==NULL){
				utils.msgbox(hWndDlg,MB_ICONEXCLAMATION,COMMON_NAME,"请先打开一个串口设备!");
				DestroyWindow(hWndDlg);
				hWndPinCtrl = NULL;
				return 0;
			}
			if(!GetCommConfig(msg.hComPort,&cconfig,&size)){
				utils.msgerr(hWndDlg,"获取串口配置时错误");
				DestroyWindow(hWndDlg);
				hWndPinCtrl = NULL;
				return 0;
			}
			hDtr = GetDlgItem(hWndDlg,IDC_CBO_PINCTRL_DTR);
			hRts = GetDlgItem(hWndDlg,IDC_CBO_PINCTRL_RTS);
			for(;;){
				int i;
				for(i=0; i<3; i++){
					ComboBox_AddString(hDtr,sdtr[i]);
					ComboBox_AddString(hRts,srts[i]);
				}
				ComboBox_SetCurSel(hDtr,cconfig.dcb.fDtrControl);
				ComboBox_SetCurSel(hRts,cconfig.dcb.fRtsControl);
				break;
			}
			//...
			hWndPinCtrl = hWndDlg;
			SetWindowPos(hWndDlg,0,axisx,axisy,0,0,SWP_NOSIZE|SWP_NOZORDER);
			//if(axisx==0 && axisy==0) utils.center_window(hWndDlg,msg.hWndMain);
		}
	}
	UNREFERENCED_PARAMETER(lParam);
	return 0;
}

int ShowPinCtrl(void)
{
	if(hWndPinCtrl){
		SetWindowPos(hWndPinCtrl,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		return 0;
	}
	//DialogBoxParam(msg.hInstance,MAKEINTRESOURCE(IDD_DLG_PINCTRL),NULL,PinCtrlDlgProc,0);
	hWndPinCtrl = CreateDialogParam(msg.hInstance,MAKEINTRESOURCE(IDD_DLG_PINCTRL),msg.hWndMain,PinCtrlDlgProc,0);
	ShowWindow(hWndPinCtrl,SW_SHOW);
	SendMessage(msg.hWndMain,WM_APP+0,0,(LPARAM)hWndPinCtrl);
	return 0;
}
