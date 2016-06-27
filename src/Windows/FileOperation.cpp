#include "stdafx.h"

namespace Common{



	c_binary_file::~c_binary_file()
	{
		close();
	}

	bool c_binary_file::open(const char* fn, const char* mode)
	{
		_fn = fn;
		_fp = ::fopen(_fn.c_str(), mode);
		return !!_fp;
	}


	bool c_binary_file::close()
	{
		bool r = true;
		if (_fp){
			r = ::fclose(_fp)==0;
			if (r == true){
				_fp = NULL;
			}
		}
		return r;
	}

	int c_binary_file::seek(int offset, int origin)
	{
		SMART_ASSERT(_fp != NULL).Fatal();
		return ::fseek(_fp, offset, origin);
	}

	bool c_binary_file::read(unsigned char* buf, int size)
	{
		SMART_ASSERT(_fp != NULL).Fatal();
		return ::fread(buf, 1, size, _fp) == size;
	}

	bool c_binary_file::write(unsigned char* buf, int size)
	{
		SMART_ASSERT(_fp != NULL).Fatal();
		return ::fwrite(buf, 1, size, _fp) == size;
	}

	int c_binary_file::tell()
	{
		SMART_ASSERT(_fp != NULL).Fatal();
		return ::ftell(_fp);
	}

	void c_binary_file::flush()
	{
		SMART_ASSERT(_fp != NULL).Fatal();
		::fflush(_fp);
	}

	const std::string& c_binary_file::get_fn()
	{
		return _fn;
	}

}
