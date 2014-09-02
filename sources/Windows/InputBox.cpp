#include "stdafx.h"
#include "../res/resource.h"

namespace Common{


	int c_input_box::do_modal(HWND hParent, i_input_box* piib)
	{
		_piib = piib;
		if (_piib){
			_piib->set_notifier(this);
			_piib->set_this(this);
			_prompt_str = _piib->get_prompt_text();
			_enter_string = _piib->get_enter_text();
		}
		assert(::IsWindow(hParent));
		Create(hParent, MAKEINTRESOURCE(IDD_INPUTBOX));
		CenterWindow();
		ShowModal();
		return _dlg_code;
	}

	LRESULT c_input_box::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
			_hPrompt = ::GetDlgItem(m_hWnd, IDC_INPUTBOX_PROMPT);
			_hEnter  = ::GetDlgItem(m_hWnd, IDC_INPUTBOX_ENTERTEXT);
			_hOk     = ::GetDlgItem(m_hWnd, IDOK);
			_hCancel = ::GetDlgItem(m_hWnd, IDCANCEL);

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
			::DestroyWindow(m_hWnd);
			return 0;

		case WM_COMMAND:
		{
			HWND hChild = (HWND)lParam;
			int code = HIWORD(wParam);
			int id = LOWORD(wParam);

			if (hChild && code == BN_CLICKED){
				if (id == IDOK){

					if (!call_interface())
						return 0;

					_dlg_code = IDOK;
					::DestroyWindow(m_hWnd);
					return 0;
				}
				else if (id == IDCANCEL){
					if (_piib && !_piib->try_close()){
						::SetFocus(_hEnter);
						return 0;
					}
					_dlg_code = IDCANCEL;
					::DestroyWindow(m_hWnd);
					return 0;
				}
			}
			break;			
		}
		}
		return __super::HandleMessage(uMsg, wParam, lParam, bHandled);
	}

	c_input_box::c_input_box()
		: _prompt_str(0)
		, _enter_string(0)
		, _dlg_code(IDCANCEL)
		, _piib(0)
		, _pstr(0)
	{

	}

	c_input_box::~c_input_box()
	{
		if (_pstr) delete[] _pstr;
	}

	bool c_input_box::ResponseDefaultKeyEvent(HWND hChild, WPARAM wParam)
	{
		if (wParam == VK_ESCAPE){
			_dlg_code = IDCANCEL;
			Close();
			return true;
		}
		else if (wParam == VK_RETURN){
			if (hChild == _hOk || hChild == _hEnter){
				if (call_interface()){
					_dlg_code = IDOK;
					::DestroyWindow(m_hWnd);
					return true;
				}
			}
			return false;
		}
		return false;
	}

	bool c_input_box::test_get_int_value()
	{
		int value;
		BOOL bTranslated;

		value = ::GetDlgItemInt(m_hWnd, IDC_INPUTBOX_ENTERTEXT, &bTranslated, TRUE);
		if (!bTranslated) return false;

		_i = value;
		return true;
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

}
