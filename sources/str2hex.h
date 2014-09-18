#pragma once

namespace Common{
	class c_str2hex_dlg : public CWnd
	{
	public:
		virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual void OnFinalMessage(HWND hWnd) override { __super::OnFinalMessage(hWnd); delete this; }

	protected:
		static int axisx, axisy;
	};
}
