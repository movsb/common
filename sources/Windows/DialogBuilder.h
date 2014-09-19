#pragma once

namespace Common{
	class c_dialog_builder : public CWnd
	{
	public:
		c_dialog_builder();

	private:
		virtual LPCTSTR		GetWindowClassName() const { return get_class_name(); }
		virtual LPCTSTR		GetWndName() const { return get_window_name(); }
		virtual DWORD		GetWndExStyle() const { return get_window_ex_style(); }
		virtual UINT		GetClassStyle() const { return get_class_style(); }
		virtual DWORD		GetWndStyle() const { return get_window_style(); }
		virtual HBRUSH		GetClassBrush() const override { return get_class_brush(); }
		virtual LRESULT		HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual void		OnFinalMessage(HWND hWnd) { on_final_message(hWnd); }

	protected:
		virtual void		response_key_event(WPARAM vk);
		virtual void		end_dialog() { ::EndDialog(m_hWnd, 0); }
		virtual LRESULT		handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual void		on_final_message(HWND hwnd) { CWnd::OnFinalMessage(hwnd); }
		virtual LRESULT		on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code) { return 0; }
		virtual LPCTSTR		get_class_name() const { return "c_dialog_builder"; }
		virtual UINT		get_class_style() const { return 0; }
		virtual HBRUSH		get_class_brush() const { return (HBRUSH)(COLOR_WINDOW); }
		virtual LPCTSTR		get_window_name() const { return get_class_name(); }
		virtual DWORD		get_window_style() const { return WS_OVERLAPPEDWINDOW; }
		virtual DWORD		get_window_ex_style() const { return 0; }
		virtual LPCTSTR		get_skin_xml() const = 0;


	protected:
		SdkLayout::CSdkLayout _layout;
	};
}
