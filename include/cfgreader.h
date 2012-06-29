#ifndef TP_CFGREADER_H_INCLUDED
#define TP_CFGREADER_H_INCLUDED

#include <map>
#include <string>
#include <stdio.h>
#include "auto_release.h"

class cfgreader
{
public:
	void open(std::wstring inifile);
private:
	typedef std::map<std::wstring, std::wstring> strstrmap_t;
	typedef std::map<std::wstring, strstrmap_t> cfgmap_t;

	cfgmap_t m_cfg;
};

inline void cfgreader::open(std::wstring inifile)
{
	FILE* fp = NULL;
	errno_t ret = _wfopen_s(fp, inifile.c_str(), L"rt");
	tp::throw_stderr_when(ret != 0);
	ON_LEAVE_1(fclose(fp), FILE*, fp);

	wchar_t line[4096] = {};
	strstrmap_t* valmap = NULL;
	while (fgetws(line, _countof(line), fp))
	{
		if (line[0] == L'[')
		{
			wchar_t* p = wcschr(line, L']');
			tp::throw_when(!p, L"format error");
			std::wstring section(line + 1, p - line - 1);
			cfgmap_t::iterator it = m_cfg.find(section);
			if (it == m_cfg.end())
			{
				it = m_cfg.insert(cfgmap_t::value_type(section, strstrmap_t())).first;
			}
			valmap = &(it->second);
			continue;
		}


	}
}



#endif