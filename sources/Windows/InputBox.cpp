#include "stdafx.h"

namespace Common{

	LRESULT c_input_box::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
			CenterWindow();

			_hPrompt = *_layout.FindControl("prompt");
			_hEnter = *_layout.FindControl("enter");
			_hOk = *_layout.FindControl("btnok");
			_hCancel = *_layout.FindControl("btncancel");

			::SetWindowText(_hPrompt, _prompt_str ? _prompt_str : _T(""));
			::SetWindowText(_hEnter, _enter_string ? _enter_string : _T(""));

			::SetFocus(_hEnter);

			return FALSE;

		case WM_CLOSE:
			if (_piib && !_piib->try_close()){
				::SetFocus(_hEnter);
				return 0;
			}
			_dlg_code = IDCANCEL;
			break;
		}
		return __super::handle_message(uMsg, wParam, lParam, bHandled);
	}

	c_input_box::c_input_box(i_input_box* piib)
		: _prompt_str(0)
		, _enter_string(0)
		, _dlg_code(IDCANCEL)
		, _piib(piib)
		, _pstr(0)
	{
		SMART_ASSERT(_piib != nullptr);

		_piib->set_notifier(this);
		_piib->set_this(this);
		_prompt_str = _piib->get_prompt_text();
		_enter_string = _piib->get_enter_text();
	}

	c_input_box::~c_input_box()
	{
		if (_pstr) delete[] _pstr;
	}

	bool c_input_box::test_get_int_value()
	{
		int value;
		if (sscanf(_pstr, "%d", &value) == 1){
			_i = value;
			return true;
		}
		else{
			return false;
		}
	}

	int c_input_box::get_int_value()
	{
		return _i;
	}

	std::string c_input_box::get_string_value()
	{
		if (_pstr) return std::string(_pstr);
		return "";
	}

	bool c_input_box::call_interface()
	{
		int len = ::GetWindowTextLength(_hEnter) + 1;
		char* p = new char[len];
		::GetWindowText(_hEnter, p, len);
		if (_pstr) delete[] _pstr;
		_pstr = p;

		if (_piib){
			if (!_piib->check_valid(_pstr)){
				::SetFocus(_hEnter);
				return false;
			}
		}

		return true;
	}

	LPCTSTR c_input_box::get_skin_xml() const
	{
		return
			R"feifei(
<Window size="230,70">
	<Font name = "Î¢ÈíÑÅºÚ" size = "12" default = "true" />
	<Font name = "Î¢ÈíÑÅºÚ" size = "12"/>
	<Vertical>
		<Horizontal inset="5,5,5,5">
			<Vertical inset="5,5,5,5">
				<Static name="prompt"/>
				<Edit name="enter" height="20"/>
			</Vertical>
			<Control width="10" />
			<Vertical width="50" height="60">
				<Button name="btnok" text="È·¶¨" />
				<Control height="5"/>
				<Button name="btncancel" text="È¡Ïû" />
			</Vertical>
		</Horizontal>
	</Vertical>
</Window>
		)feifei";
	}

	LRESULT c_input_box::on_command_ctrl(HWND hwnd, const SdkLayout::CTinyString& name, int code)
	{
		if (name == "btnok"){
			if (!call_interface())
				return 0;

			_dlg_code = IDOK;
			end_dialog();
			return 0;
		}
		else if (name == "btncancel"){
			if (!_piib->try_close()){
				::SetFocus(_hEnter);
				return 0;
			}

			_dlg_code = IDCANCEL;
			end_dialog();
			return 0;
		}
		return 0;
	}

	void c_input_box::response_key_event(WPARAM vk)
	{
		if (vk == VK_RETURN){
			if (call_interface()){
				_dlg_code = IDOK;
				end_dialog();
			}
		}
		return __super::response_key_event(vk);
	}

}
