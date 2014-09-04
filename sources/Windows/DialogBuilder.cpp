#include "stdafx.h"

namespace Common{
	void c_dialog_builder::show(HWND hParent)
	{
		Create(hParent, "", GetDialogStyle(), 0);
		SIZE sz = _layout.GetPostSize();
		ResizeClient(sz.cx, sz.cy);
		CenterWindow();
		ShowModal();
	}

	LRESULT c_dialog_builder::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch (uMsg)
		{
		case WM_SIZE:
			_layout.ResizeLayout();
			return 0;
		case WM_HSCROLL:
		case WM_VSCROLL:
			_layout.ProcessScrollMessage(uMsg, wParam, lParam);
			return 0;
		case WM_CREATE:
			_layout.SetDlgGetID(this);
			_layout.SetLayout(m_hWnd, get_skin_xml());
			SMART_ASSERT(_layout.GetRoot() != NULL).Fatal();
			return 0;
		case WM_LBUTTONDOWN:
			SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0);
			return 0;
		}
		return __super::HandleMessage(uMsg, wParam, lParam, bHandled);
	}

	LPCTSTR c_dialog_builder::get_skin_xml() const
	{
		assert(0);
		return "";
	}
}
