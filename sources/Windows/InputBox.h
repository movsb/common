#pragma once

namespace Common{
	class c_input_box;

	class i_input_box
	{
	public:
		virtual bool try_close() = 0;
		virtual bool check_valid(const char* str) = 0;
		virtual void set_notifier(i_notifier* notifier) = 0;
		virtual void set_this(c_input_box* that) = 0;
		virtual const char* get_enter_text() = 0;
		virtual const char* get_prompt_text() = 0;
	};

	class c_input_box : public c_dialog_builder
	{
	public:
		c_input_box(i_input_box* piib);
		~c_input_box();

		bool test_get_int_value();
		int get_int_value();
		std::string get_string_value();
		int get_dlg_code() const { return _dlg_code; }

	private:
		virtual LPCTSTR get_skin_xml() const override;

	protected:
		virtual LRESULT		handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;
		virtual LRESULT		on_command_ctrl(HWND hwnd, const SdkLayout::CTinyString& name, int code) override;
		bool call_interface();

	protected:
		i_input_box* _piib;
		int _dlg_code;
		const char* _prompt_str;
		const char* _enter_string;
		const char* _pstr;
		int _i;

		HWND _hPrompt, _hEnter, _hOk, _hCancel;
	};
}
