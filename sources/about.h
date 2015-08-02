#pragma once

namespace Common{

#define COMMON_NAME			"Com Monitor"
#define COMMON_VERSION		"1.19"

#ifdef _DEBUG 
	#define COMMON_NAME_AND_VERSION COMMON_NAME " " COMMON_VERSION " - Debug Mode"
#else
	#define COMMON_NAME_AND_VERSION COMMON_NAME " " COMMON_VERSION " "
#endif

	class c_about_dlg : public c_dialog_builder
	{
	protected:
		virtual LRESULT		handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual void		on_final_message(HWND hwnd) { __super::on_final_message(hwnd); delete this; }
		virtual LPCTSTR		get_skin_xml() const override;
		virtual LRESULT		on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code) override;
		virtual LPCTSTR		get_window_name() const;

	private:
		static const char* about_str;
		static const char* soft_name;
	};
}
