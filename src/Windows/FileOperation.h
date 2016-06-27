#pragma once

namespace Common{
	// 只是非常简单的封装
	class c_binary_file
	{
	public:
		c_binary_file(){ _fp = NULL; }
		~c_binary_file();

		bool open(const char* fn, const char* mode);
		bool close();
		int seek(int offset, int origin);
		bool read(unsigned char* buf, int size);
		bool write(unsigned char* buf, int size);
		int tell();
		void flush();
		const std::string& get_fn();

	protected:
		std::string _fn;
		FILE* _fp;
	};
}
