#include "stdafx.h"

namespace Common{
	bool CComConfig::Load(const char* str)
	{
		if (!str || !*str)
			return false;

		const char* p = str;
		for (bool loop = true; loop;){
			std::string k, v, c;
			_skip_ws(p);
			if (*p == '\n'){
				auto item = new CComConfigItem("", "", "");
				list_insert_tail(&_head, &item->list_entry);
				_skip_ln(p);
				continue;
			}
			else if (*p == ';' || *p == '#'){
				_read_cmt(p, &c);

				auto item = new CComConfigItem("", "", c.c_str());

				list_insert_tail(&_head, &item->list_entry);
				_skip_ln(p);
				goto _loop;
			}
			else if (*p == '\0'){
				loop = false;
				goto _loop;
			}
			else{
				_read_str(p, &k);
				_skip_ws(p);

				if (*p == '='){
					++p;
					_skip_ws(p);

					if (*p == '"'){
						++p;
						_read_str(p, &v, true);
						if (*p == '"'){
							++p;
							_skip_ws(p);
							if (*p == ';' || *p == '#'){
								_read_cmt(p, &c);
							}
							_skip_ln(p);

							auto item = new CComConfigItem(k.c_str(), v.c_str(), c.c_str());
							list_insert_tail(&_head, &item->list_entry);
							goto _loop;
						}
						else{
							_skip_ln(p);
							goto _loop;
						}
					}
					else if (*p == ';' || *p == '#'){
						_read_cmt(p, &c);
						auto item = new CComConfigItem(k.c_str(), v.c_str(), c.c_str());
						list_insert_tail(&_head, &item->list_entry);
						_skip_ln(p);
						goto _loop;
					}
					else{
						_read_str(p, &v, false);
						_skip_ws(p);
						if (*p == ';' || *p == '#'){
							_read_cmt(p, &c);
						}
						auto item = new CComConfigItem(k.c_str(), v.c_str(), c.c_str());
						list_insert_tail(&_head, &item->list_entry);
						_skip_ln(p);
						goto _loop;
					}
				}
				else{
					_skip_ln(p);
					goto _loop;
				}
			}
		_loop:
			;
		}
		return true;
	}

	bool CComConfig::LoadFile(const char* file)
	{
		_file = file;

		FILE* fp = fopen(file, "rb");
		if (!fp) return false;

		char* buf;
		int sz, sz2;

		fseek(fp, 0, SEEK_END);
		sz = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (sz == 0 || sz > 1 << 20){
			fclose(fp);
			return false;
		}

		buf = new char[sz + 1];
		buf[sz + 1 - 1] = '\0';

		sz2 = fread(buf, 1, sz, fp);
		fclose(fp);

		if (sz2 != sz){
			delete[] buf;
			return false;
		}

		bool r = Load(buf);
		delete[] buf;

		return r;
	}

	void CComConfig::_skip_ws(const char*& p)
	{
		while (*p && (*p == ' ' || *p == '\t' || *p == '\r'))
			++p;
	}

	void CComConfig::_skip_ln(const char*& p)
	{
		while (*p && *p != '\n')
			++p;
		if (*p == '\n')
			++p;
	}

	void CComConfig::_read_str(const char*& p, std::string* s, bool in_quot)
	{
		std::vector<char> chars;
		while (*p){
			if (*p == '"'){
				goto _exit;
				return;
			}
			else if (*p == '\\'){
				if (p[1] == '"'){
					chars.push_back(p[1]);
					p += 2;
					continue;
				}
				else if (p[1] == '\r' || p[1] == '\n'){
					if (p[1] == '\r' && p[2] == '\n')
						++p;
					p += 2;
					continue;
				}
				else{
					chars.push_back(p[0]);
					p += 1;
					continue;
				}
			}
			else if (*p == ' ' || *p == '\t' || *p == '='){
				if (!in_quot){
					goto _exit;
				}
				else{
					chars.push_back(*p);
					++p;
					continue;
				}
			}
			else if (*p == '\r' || *p == '\n'){
				goto _exit;
			}
			else{
				if (*p == ';' || *p == '#'){
					if (!in_quot){
						goto _exit;
					}
				}
				chars.push_back(*p);
				++p;
				continue;
			}
		}
	_exit:
		chars.push_back('\0');
		*s = std::string(reinterpret_cast<char*>(&chars[0]));
		return;
	}

	void CComConfig::_read_cmt(const char*& p, std::string* cmt)
	{
		std::vector<char> _cmtv;
		while (*p && *p != '\n' && *p != '\r'){
			_cmtv.push_back(*p);
			p++;
		}
		_cmtv.push_back('\0');
		*cmt = std::string(reinterpret_cast<char*>(&_cmtv[0]));
	}

	CComConfig::CComConfig()
	{
		list_init(&_head);
	}

	bool CComConfig::SaveFile(const char* file)
	{
		std::ofstream of;
		of.open(file ? file : _file.c_str(), std::ios_base::binary);
		if (!of.is_open())
			return false;

		while (!list_is_empty(&_head)){
			list_s* li = list_remove_head(&_head);
			CComConfigItem* item = list_data(li, CComConfigItem, list_entry);
			if (item->key().size()){
				of << item->key() << " = ";
				if (item->val().find(' ') != std::string::npos){
					of << "\"" << item->val() << "\"";
				}
				else{
					of << item->val();
				}
			}
			if (item->cmt().size()){
				if (item->key().size()) of << "\t";
				of << item->cmt();
			}
			of << "\r\n";
			delete item;
		}
		of.close();
		return true;
	}

	CComConfig::~CComConfig()
	{
		while (!list_is_empty(&_head)){
			list_s* li = list_remove_head(&_head);
			CComConfigItem* item = list_data(li, CComConfigItem, list_entry);
			delete item;
		}
	}

	CComConfigItem* CComConfig::get_key(const char* key)
	{
		for (list_s* p = _head.next; p != &_head; p = p->next){
			auto item = list_data(p, CComConfigItem, list_entry);
			if (item->key() == key){
				return item;
			}
		}
		return nullptr;
	}

	void CComConfig::set_key(const char* key, const char* val)
	{
		auto item = get_key(key);
		if (item == nullptr){
			item = new CComConfigItem(key, val, "");
			list_insert_tail(&_head, &item->list_entry);
		}
		else{
			item->set_str(val);
		}
	}

	void CComConfig::set_key(const char* key, bool val)
	{
		auto item = get_key(key);
		if (item == nullptr){
			item = new CComConfigItem(key, val ? "true" : "false", "");
			list_insert_tail(&_head, &item->list_entry);
		}
		else{
			item->set_bool(val);
		}
	}

	void CComConfig::set_key(const char* key, int val)
	{
		auto item = get_key(key);
		if (item == nullptr){
			item = new CComConfigItem(key, int2str(val).c_str(), "");
			list_insert_tail(&_head, &item->list_entry);
		}
		else{
			item->set_str(int2str(val).c_str());
		}
	}

	std::string CComConfig::int2str(int i)
	{
		char buf[32];
		sprintf(buf, "%d", i);
		return buf;
	}

	int CComConfig::str2int(const std::string& s)
	{
		int i;
		if (sscanf(s.c_str(), "%d", &i) == 1)
			return i;
		else
			return 0;
	}


	void CComConfigItem::set_str(const char* val)
	{
		_val = val;
	}

	void CComConfigItem::set_int(int i)
	{
		_val = CComConfig::int2str(i);
	}

	void CComConfigItem::set_bool(bool b)
	{
		_val = b ? "true" : "false";
	}

	void CComConfigItem::set_cmt(const char* cmt)
	{
		_cmt = cmt;
	}

	const std::string& CComConfigItem::get_str()
	{
		return _val;
	}

	int CComConfigItem::get_int()
	{
		return CComConfig::str2int(_val);
	}

	bool CComConfigItem::get_bool()
	{
		return _val == "true";
	}

}
