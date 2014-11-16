#pragma once

namespace Common{

	class c_cs_console : public c_dialog_builder
	{
	public:
		c_cs_console()
			: _show_output(true)
		{}

		void append_text(const char* s);			

	protected:
		//virtual void		response_key_event(WPARAM vk);
		virtual void		end_dialog() { ::EndDialog(m_hWnd, 0); }
		virtual LRESULT		handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual void		on_final_message(HWND hwnd);
		virtual LRESULT		on_notify_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code, NMHDR* hdr);
		virtual LRESULT		on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code);
		virtual LRESULT		on_menu(int id) { return 0; }
		virtual LPCTSTR		get_window_name() const { return "C”Ô—‘Ω≈±æøÿ÷∆Ã®"; }
		virtual DWORD		get_window_style() const { return __super::get_window_style(); }
		virtual LPCTSTR		get_skin_xml() const;

	private:
		bool _show_output;
	};

	extern c_cs_console* __this_cs_console;

}
