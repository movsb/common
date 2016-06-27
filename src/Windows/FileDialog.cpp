#include "stdafx.h"

namespace Common{


	c_file_dlg::c_file_dlg()
	{
		::memset(&_ofn, 0, sizeof(_ofn));
		_ofn.lStructSize = sizeof(_ofn);
		_ofn.hInstance = theApp.instance();
		_ofn.nMaxFile = sizeof(_buffer);
		_ofn.lpstrFile = _buffer;
		*_buffer = '\0';

	}

	c_file_open_dlg::c_file_open_dlg()
	{
		_ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST;
	}

	bool c_file_open_dlg::do_modal(HWND hOwner)
	{
		_ofn.hwndOwner = hOwner;
		return !!::GetOpenFileName(&_ofn);
	}



	c_file_save_dlg::c_file_save_dlg()
	{
		_ofn.Flags = OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_EXPLORER | OFN_NOREADONLYRETURN | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	}

	bool c_file_save_dlg::do_modal(HWND hOwner)
	{
		_ofn.hwndOwner = hOwner;
		return !!::GetSaveFileName(&_ofn);
	}

}
