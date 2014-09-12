#include "stdafx.h"

namespace Common{

	c_the_app::c_the_app()
	{
		_hinst = ::GetModuleHandle(0);

		char buf[MAX_PATH]={0};
		::GetModuleFileName(_hinst, buf, __ARRAY_SIZE(buf));
		
		char* p = strrchr(buf, '\\');
		if(!p) p = strrchr(buf, '/');
		if(p) *p = '\0';
		
		_path = buf;
	}

	HINSTANCE c_the_app::instance()
	{
		return _hinst;
	}

	std::string c_the_app::path()
	{
		return _path;
	}

	c_the_app::operator HINSTANCE()
	{
		return _hinst;
	}

}

