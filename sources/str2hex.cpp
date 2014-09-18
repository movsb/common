#include "StdAfx.h"
#include "../res/resource.h"

static char* __THIS_FILE__ = __FILE__;

namespace Common{
	int c_str2hex_dlg::axisx = -1;
	int c_str2hex_dlg::axisy = -1;

	LRESULT c_str2hex_dlg::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
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
		case WM_INITDIALOG:
			{
				SetWindowPos(m_hWnd,0,axisx,axisy,0,0,SWP_NOSIZE|SWP_NOZORDER);
				if(axisx==-1 && axisy==-1) 
					CenterWindow();
				return 0;
			}
		}
		return CWnd::HandleMessage(uMsg, wParam, lParam, bHandled);
	}
}
