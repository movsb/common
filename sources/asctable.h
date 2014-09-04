#pragma once

namespace Common{
	class c_asctable_dlg : public CWnd
	{
	public:
		c_asctable_dlg();
		void show(HWND hParent);
		virtual LPCTSTR GetWindowClassName() const {return "asctable";};
		virtual UINT GetClassStyle() const { return __super::GetClassStyle() & ~CS_DBLCLKS; }
		virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual void OnFinalMessage(HWND hWnd) override { __super::OnFinalMessage(hWnd); delete this; }

	protected:
		static int axisx, axisy;
		HFONT _hFont;
		static int _fgcolor;
		static int _bgcolor;
	};
}
