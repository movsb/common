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
		case WM_INITDIALOG:
		{
			HWND hParent = GetWindowOwner(m_hWnd);
			SendMessage(WM_SETICON, ICON_BIG, LPARAM(::SendMessage(hParent, WM_GETICON, ICON_BIG, 0)));
			SendMessage(WM_SETICON, ICON_SMALL, LPARAM(::SendMessage(hParent, WM_GETICON, ICON_SMALL, 0)));
			_layout.SetLayout(m_hWnd, get_skin_xml());
			SMART_ASSERT(_layout.GetRoot() != NULL).Fatal();

			SetWindowLongPtr(m_hWnd, GWL_STYLE, get_window_style());
			if (get_window_style() & WS_CHILD) ::SetParent(m_hWnd, hParent);
			break;
		}
		case WM_COMMAND:
		{
			HWND hwnd = HWND(lParam);
			int id = LOWORD(wParam);
			int code = HIWORD(wParam);

			if (hwnd == nullptr &&code == 0){
				if (id == 2){
					response_key_event(VK_ESCAPE);
					return 0;
				}
				else if (id == 1){
					response_key_event(VK_RETURN);
					return 0;
				}
			}

			if (hwnd != NULL){
				SdkLayout::CControlUI* pControl = _layout.FindControl(hwnd);
				if (pControl){
					return on_command_ctrl(hwnd, pControl, code);
				}
			}
		}
		case WM_NOTIFY:
		{
			auto hdr = reinterpret_cast<NMHDR*>(lParam);
			auto ctrl = _layout.FindControl(hdr->hwndFrom);
			if (!ctrl) break;
			return on_notify_ctrl(hdr->hwndFrom, ctrl, hdr->code ,hdr);
		}
		break;
		}
		return handle_message(uMsg, wParam, lParam, bHandled);
	}

	LRESULT c_dialog_builder::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		return __super::HandleMessage(uMsg, wParam, lParam, bHandled);
	}

	void c_dialog_builder::response_key_event(WPARAM vk)
	{
		if (vk == VK_ESCAPE){
			Close();
		}
	}

}
