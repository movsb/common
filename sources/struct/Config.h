#pragma once

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus

#include <map>
#include <string>
#include <vector>
#include <cstdio>

class CComConfig
{
public:
	CComConfig(){}
	bool			Load(const char* str);
	bool			LoadFile(const char* file);
	int				GetInt(const char* name, int def);
	bool			GetBool(const char* name, bool def);
	std::string		GetString(const char* name, const char* def);
	bool			Has(const char* name){return _has(name);}


protected:
	void _skip_ws(const char*& p);
	void _skip_ln(const char*& p);
	void _read_str(const char*& p, std::string* s, bool in_quot=false);

	bool _has(const char* name)
	{
		return _maps.count(name)>0;
	}
	std::string _get(const char* name)
	{
		return _maps[name];
	}

protected:
	std::map<std::string, std::string> _maps;

private:
	CComConfig operator=(const CComConfig&);
	CComConfig(const CComConfig&);
};

#endif // __cplusplus

#ifdef __cplusplus
typedef CComConfig comconfig;
#else
typedef struct comconfig comconfig;
#endif

#ifdef __cplusplus
extern "C" {
#endif

	comconfig*	config_create(const char* s, int bfile);
	void		config_close(comconfig* cfg);
	int			config_getint(comconfig* cfg, const char* name, int def);
	int			config_getbool(comconfig* cfg, const char* naem, int def);
	char*		config_getstr(comconfig* cfg, const char* name, const char* def);
	int			config_has(comconfig* cfg, const char* name);
#ifdef __cplusplus
}
#endif

#endif //!__CONFIG_H__
