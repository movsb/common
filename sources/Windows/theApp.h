#pragma once

namespace Common{
	class c_the_app
	{
	public:
		c_the_app();
		HINSTANCE instance();
		std::string path();

	private:
		HINSTANCE   _hinst;
		std::string _path;
	};
}

extern Common::c_the_app theApp;
