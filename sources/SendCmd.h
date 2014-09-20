#pragma once

namespace Common{
	bool sendcmd_try_load_xml(HWND hOwner, LPCTSTR xml_file);

	class c_send_cmd_item_ui : public SdkLayout::CControlUI
	{
	public:
		virtual LPCTSTR GetClass() const override{
			return GetClassStatic();
		}
		static LPCTSTR GetClassStatic() {
			return _T("send_cmd_item");
		}
		void SetManager(SdkLayout::CPaintManagerUI* mgr) override{
			m_pManager = mgr;
		}
	};

	class c_send_cmd_item : public c_dialog_builder
	{
	public:
		enum class scimsg{
			__start = WM_USER+0,
			item_expand,		// wParam: bool, lParam: not used
		};
	protected:
		virtual DWORD		get_window_style() const override;
		virtual void		response_key_event(WPARAM vk) override;
		virtual LRESULT		handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;
		virtual LRESULT		on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code) override;
		virtual LPCTSTR		get_skin_xml() const override;
		virtual void		on_final_message(HWND hwnd) { __super::on_final_message(hwnd); delete this; }

	protected:
		void	do_expand();

	public:
		int		get_height() const;
		void	set_name(const char* name);
		void	set_script(const char* script);

	public:
		c_send_cmd_item();
		~c_send_cmd_item();

	private:
		bool _b_expanded;
	};

	class c_send_cmd_dialog : public c_dialog_builder
	{
	public:
		c_send_cmd_dialog(tinyxml2::XMLDocument* pdoc = nullptr);
		~c_send_cmd_dialog();

	protected:
		void	_init_cmds_from_doc();
		void	_insert_new_cmd_to_ui(const tinyxml2::XMLElement* cmd);

	protected:
		virtual void		response_key_event(WPARAM vk) override;
		virtual LRESULT		handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;
		virtual LRESULT		on_command_ctrl(HWND hwnd, SdkLayout::CControlUI* ctrl, int code) override;
		virtual LPCTSTR		get_skin_xml() const override;

	private:
		tinyxml2::XMLDocument*	_xml;
	};
}
