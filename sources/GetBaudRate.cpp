#include <Windows.h>
#include <windowsx.h>
#include <cstdio>
#include <string>

#include "GetBaudRate.h"
#include "../res/resource.h"
#include "struct/Thunk.h"

EXTERN_C void center_window(HWND hWnd, HWND hWndOwner);

class AGetBaudRate
{
public:
	AGetBaudRate(HWND hParent)
	{
		m_hParent=hParent;
		Create(hParent);
	}
	int GetValue() const {return m_value;}
	int GetDlgCode() const{return m_dlgcode;}
private:
	HWND m_hParent;
	AThunk m_Thunk;
	HWND m_hWnd;
	int m_value;
	int m_dlgcode;
	void Create(HWND hParent)
	{
		DLGPROC p = (DLGPROC)m_Thunk.Stdcall(this,&AGetBaudRate::WindowProc);
		m_dlgcode = DialogBox(GetModuleHandle(0),MAKEINTRESOURCE(IDD_GETBR),hParent,p);
	}
	INT_PTR __stdcall WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_CLOSE:
			EndDialog(hWnd,IDCANCEL);
			SetDlgMsgResult(hWnd,WM_COMMAND,0);
			return TRUE;
		case WM_INITDIALOG:
			{
				m_hWnd=hWnd;
				center_window(hWnd,m_hParent);
				SetFocus(GetDlgItem(hWnd,IDC_EDIT1));
				ShowWindow(hWnd,SW_SHOWNORMAL);
				return FALSE;
			}
		case WM_COMMAND:
			{
				int id = (short)LOWORD(wParam);
				int code = (short)HIWORD(wParam);
				HWND hCtrl = (HWND)lParam;

				switch(id)
				{
				case IDCANCEL:
					EndDialog(hWnd,IDCANCEL);
					SetDlgMsgResult(hWnd,WM_COMMAND,0);
					return TRUE;
				case IDOK:
					{
						char s[128]={0};
						int value=0;
						GetDlgItemText(hWnd,IDC_EDIT1,s,sizeof(s)-1);
						sscanf(s,"%i",&value);

						std::string old = s;
						sprintf(s,"%d",value);
						if(old != s){
							MessageBox(m_hWnd,"无效的值~",0,MB_ICONEXCLAMATION);
							return 0;
						}
						m_value = value;
						EndDialog(hWnd,IDOK);
						SetDlgMsgResult(hWnd,WM_COMMAND,0);
						return TRUE;
					}
					
				}
			}
		}
		return 0;
	}
};

EXTERN_C BOOL GetNewBR(HWND hParent,int* value)
{
	AGetBaudRate br(hParent);
	if(br.GetDlgCode()==IDOK){
		*value=br.GetValue();
		return TRUE;
	}
	return FALSE;
}
