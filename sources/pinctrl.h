#pragma once

namespace Common{
	class c_pinctrl_dlg : public CWnd
	{
	public:
		virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);

	private:
		static int axisx, axisy;
	};
}
