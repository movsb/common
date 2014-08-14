
#include "Config.h"

bool CComConfig::Load(const char* str)
{
	if (!str || !*str)
		return false;

	const char* p = str;
	for (bool loop = true; loop;){
		std::string k, v;
		_skip_ws(p);
		if (*p == '\n'){
			_skip_ln(p);
			continue;
		}
		else if (*p == ';' || *p == '#'){
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
						_maps[k] = v;
						++p;
						_skip_ln(p);
						goto _loop;
					}
					else{
						_skip_ln(p);
						goto _loop;
					}
				}
				else if (*p == ';' || *p == '#'){
					_maps[k] = "";
					_skip_ln(p);
					goto _loop;
				}
				else{
					_read_str(p, &v, false);
					_maps[k] = v;
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

bool CComConfig::LoadFile( const char* file )
{
	FILE* fp = fopen(file,"rb");
	if(!fp) return false;

	char* buf;
	int sz, sz2;

	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(sz == 0 || sz > 1<<20){
		fclose(fp);
		return false;
	}

	buf = new char[sz+1];
	buf[sz+1-1] = '\0';
	
	sz2 = fread(buf, 1, sz, fp);
	fclose(fp);

	if(sz2 != sz){
		delete buf;
		return false;
	}

	bool r = Load(buf);
	delete buf;

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
			else if(p[1]=='\r' ||p[1]=='\n'){
				if(p[1]=='\r' && p[2]=='\n')
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
		else if (*p==' ' || *p=='\t' || *p=='='){
			if(!in_quot){
				goto _exit;
			}
			else{
				chars.push_back(*p);
				++p;
				continue;
			}
		}
		else if(*p == '\r' || *p == '\n'){
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

int CComConfig::GetInt(const char* name, int def)
{
	if (_has(name)){
		return strtol(_get(name).c_str(), 0, 10);
	}
	else{
		return def;
	}
}

bool CComConfig::GetBool(const char* name, bool def)
{
	if (_has(name)){
		return _stricmp(_get(name).c_str(), "true") == 0;
	}
	else{
		return def;
	}
}

std::string CComConfig::GetString(const char* name, const char* def)
{
	if (_has(name)){
		return _get(name);
	}
	else{
		return def;
	}
}

comconfig* config_create( const char* s, int bfile )
{
	CComConfig* cfg = new CComConfig;
	bool r;
	if(bfile) r = cfg->LoadFile(s);
	else      r = cfg->Load(s);

	if(r) return cfg;
	else{
		delete cfg;
		return 0;
	}
}

void config_close( comconfig* cfg )
{
	if(cfg){
		delete cfg;
	}
}

int config_getint( comconfig* cfg, const char* name, int def )
{
	return cfg 
		? cfg->GetInt(name, def)
		: def;
}

int config_getbool( comconfig* cfg, const char* naem, int def )
{
	return cfg
		? cfg->GetBool(naem, def!=0)
		: def != 0;
}

char* config_getstr( comconfig* cfg, const char* name, const char* def )
{
	std::string s;
	if(cfg) 
		s = cfg->GetString(name, def);
	else
		s = def;

	return _strdup(s.c_str());
}

int config_has( comconfig* cfg, const char* name )
{
	return cfg && cfg->Has(name);
}
