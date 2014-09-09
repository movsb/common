#include "stdafx.h"

namespace Common{

	c_dialog_builder::c_dialog_builder()
	{

	}

	LRESULT c_dialog_builder::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch (uMsg)
		{
		case WM_SIZE:
			_layout.ResizeLayout();
			break;
		case WM_HSCROLL:
		case WM_VSCROLL:
			//_layout.ProcessScrollMessage(uMsg, wParam, lParam);
			break;
		case WM_CREATE:
			_layout.SetLayout(m_hWnd, get_skin_xml());
			SMART_ASSERT(_layout.GetRoot() != NULL).Fatal();
			break;
		case WM_COMMAND:
		{
			HWND hwnd = HWND(lParam);
			int id = LOWORD(wParam);
			int code = HIWORD(wParam);

			if (hwnd != NULL){
				SdkLayout::CControlUI* pControl = _layout.FindControl(hwnd);
				if (pControl){
					return on_command_ctrl(hwnd, pControl->GetName(), code);
				}
			}

			return 0;
		}
		}
		return handle_message(uMsg, wParam, lParam);
	}

	LRESULT c_dialog_builder::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		bool b;
		return __super::HandleMessage(uMsg, wParam, lParam, b);
	}
}
