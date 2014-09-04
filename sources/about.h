#pragma once

namespace Common{

#define COMMON_NAME			"Com Monitor"
#define COMMON_VERSION		"1.18 Beta"

#ifdef _DEBUG 
	#define COMMON_NAME_AND_VERSION COMMON_NAME " " COMMON_VERSION " - Debug Mode"
#else
	#define COMMON_NAME_AND_VERSION COMMON_NAME " " COMMON_VERSION " "
#endif

	class c_about_dlg : public c_dialog_builder
	{
	protected:
		virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled);
		virtual LPCTSTR get_skin_xml() const override;
		virtual UINT get_ctrl_id(LPCTSTR name) const;
		virtual UINT GetDialogStyle() const override;

	private:
		static const char* about_str;
		static const char* soft_name;
	};
}
