#pragma once

namespace Common{
	class CComConfigItem
	{
	public:
		list_s list_entry;
		CComConfigItem(const char* key, const char* val, const char* cmt)
			: _key(key), _val(val), _cmt(cmt)
		{}

		const std::string& key() { return _key; }
		const std::string& val() { return _val; }
		const std::string& cmt() { return _cmt; }

		void set_str(const char* val);
		void set_int(int i);
		void set_bool(bool b);
		void set_cmt(const char* cmt);

		const std::string&	get_str();
		int					get_int();
		bool				get_bool();

	protected:
		std::string _key;
		std::string _val;
		std::string _cmt;

	};

	class CComConfig
	{
	public:
		CComConfig();
		~CComConfig();

		static std::string		int2str(int i);
		static int				str2int(const std::string& s);

		bool			Load(const char* str);
		bool			LoadFile(const char* file);
		bool			SaveFile(const char* file = nullptr);

		CComConfigItem*	get_key(const char* key);
		void			set_key(const char* key, const char* val);
		void			set_key(const char* key, bool val);
		void			set_key(const char* key, int val);

	protected:
		void _skip_ws(const char*& p);
		void _skip_ln(const char*& p);
		void _read_str(const char*& p, std::string* s, bool in_quot = false);
		void _read_cmt(const char*& p, std::string* cmt);

	protected:
		list_s _head;

	private:
		std::string _file;
		CComConfig operator=(const CComConfig&);
		CComConfig(const CComConfig&);
	};

}
