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

	class c_input_box : public CWnd
	{
	public:
		c_input_box();
		~c_input_box();
		int do_modal(HWND hParent, i_input_box* piib);
		void set_prompt_string(const char* s) { _prompt_str = s; }
		void set_enter_string(const char* s)  { _enter_string = s; }
		bool test_get_int_value();
		int get_int_value();
		std::string get_string_value();

	protected:
		virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;
		virtual bool ResponseDefaultKeyEvent(HWND hChild, WPARAM wParam) override;
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
