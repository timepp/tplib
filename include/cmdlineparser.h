#pragma once

#include <string>
#include <list>
#include "./auto_release.h"
#include "./exception.h"
#include "./format_shim.h"
#include "./convert.h"


namespace tp
{

class cmdline_parser
{
public:
	typedef std::list<std::wstring> strlist_t;
	enum param_type_t {param_type_int, param_type_bool, param_type_string};

	// EXCEPTIONs
	struct invalid_option : public tp::exception
	{
		std::wstring opt;
		explicit invalid_option(const std::wstring& p): opt(p)
		{
			message = opt + L": Invalid option";
		}
	};
	struct missing_option_value : public tp::exception
	{
		std::wstring opt;
		explicit missing_option_value(const std::wstring& p) : opt(p)
		{
			message = opt + L": Missing option value";
		}
	};

private:
	struct option_info
	{
		const wchar_t* short_name;
		const wchar_t* long_name;
		const wchar_t* desc;
		bool need_param;
		param_type_t param_type;
		void* value_receiver;

		int option_position;                   // The Option's position in command line, -1 indicates this option does not exists
		int param_value_int;
		bool param_value_bool;
		std::wstring param_value_string;

		option_info(const wchar_t* sname, const wchar_t* lname, bool needparam, param_type_t t, void* p)
			: short_name(sname), long_name(lname), need_param(needparam), param_type(t), value_receiver(p)
			, option_position(-1), param_value_int(0), param_value_bool(false)
		{
		}
	};

	typedef std::list<option_info> options_t;

	strlist_t m_targets;
	options_t m_options;

private:
	static bool is_white_space(wchar_t ch)
	{
		return ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n';
	}

	option_info* get_option_info(const wchar_t* opt)
	{
		for (options_t::iterator it = m_options.begin(); it != m_options.end(); ++it)
		{
			if (wcscmp(it->short_name, opt) == 0 || wcscmp(it->long_name, opt) == 0)
			{
				return &(*it);
			}
		}
		throw invalid_option(opt);
		return NULL;
	}

	const option_info* get_option_info(const wchar_t* opt) const
	{
		for (options_t::const_iterator it = m_options.begin(); it != m_options.end(); ++it)
		{
			if (wcscmp(it->short_name, opt) == 0 || wcscmp(it->long_name, opt) == 0)
			{
				return &(*it);
			}
		}
		return NULL;
	}

	void save_option(const wchar_t* opt, const wchar_t* buffer)
	{
		option_info* oi = get_option_info(opt);
		oi->param_value_string = buffer;

		if (oi->param_type == param_type_string)
		{
			if (oi->value_receiver)
			{
				*(std::wstring*)oi->value_receiver = oi->param_value_string;
			}
		}
		else if (oi->param_type == param_type_int)
		{
			oi->param_value_int = tp::cvt::to_int(buffer);
			if (oi->value_receiver)
			{
				*(int*)oi->value_receiver = oi->param_value_int;
			}
		}
		else if (oi->param_type == param_type_bool)
		{
			oi->param_value_bool = tp::cvt::to_bool(buffer);
			if (oi->value_receiver)
			{
				*(bool*)oi->value_receiver = oi->param_value_bool;
			}
		}
	}

public:
	void register_option(const wchar_t* short_name, const wchar_t* long_name, param_type_t param_type, void* value_addr)
	{
		option_info oi(short_name, long_name, true, param_type, value_addr);
		m_options.push_back(oi);
	}
	void register_switch(const wchar_t* short_name, const wchar_t* long_name, bool* value_addr)
	{
		option_info oi(short_name, long_name, false, param_type_bool, value_addr);
		m_options.push_back(oi);
	}
	void clean()
	{
		m_options.clear();
		m_targets.clear();
	}

	//! parse from command line
	//! A sample command-line:   
	//! aaa.exe --value=cc -hls -t "c d e" -- -fffc.txt
	void parse(const wchar_t* cmd_line)
	{
		/** If an option value contains spaces, it must be double-quoted.  
		 *  Double-quotes in option value is encoded with `\"'.
		 */
		strlist_t param_list;
		const wchar_t * p = cmd_line;
		while(*p)
		{
			bool in_quote = false;
			std::wstring param;
			const wchar_t *q = p;
			for(;;)
			{
				if (q[0]== L'\\' && q[1] == L'\"')
				{
					param += L'\"';
					q += 2;
					continue;
				}

				if (q[0] == L'\"')
				{
					in_quote = !in_quote;
					q++;
					continue;
				}

				if (is_white_space(q[0]) && !in_quote)
				{
					p = q+1;
					while (is_white_space(*p)) p++;
					break;
				}

				if (q[0] == L'\0')
				{
					p = q;
					break;
				}

				param += *q;
				q++;
			}

			param_list.push_back(param);
		}

		size_t argc = param_list.size();
		const wchar_t** argv = new const wchar_t* [argc];
		ON_LEAVE_1(delete[] argv, const wchar_t**, argv);

		int i = 0;
		for (strlist_t::const_iterator it = param_list.begin(); it != param_list.end(); ++it)
		{
			if (i < argc)
				argv[i] = it->c_str();
			i++;
		}

		parse(argc, argv);
	}

	//! parse from main()'s argc and argv
	void parse(size_t argc, const wchar_t* const * argv)
	{
		// clean previous results
		{
			m_targets.clear();
			for (options_t::iterator it = m_options.begin(); it != m_options.end(); ++it)
			{
				it->option_position = -1;
			}
		}

		bool in_targets = false;
		for (size_t i = 1; i < argc; i++)
		{
			const wchar_t* arg = argv[i];

			if (arg[0] != '-' || in_targets)
			{
				m_targets.push_back(arg);
				continue;
			}
			if (arg[1] == '-' && !arg[2]) // after `--', everything become `target'
			{
				in_targets = true;
				continue;
			}

			if (arg[1] == '-')
			{
				// --opt
				const wchar_t *p = wcschr(arg, L'=');
				if (p != NULL)
				{
					std::wstring param(arg + 2, static_cast<size_t>(p - arg - 2));
					save_option(param.c_str(), p+1);
				}
				else
				{
					std::wstring param = arg + 2;
					option_info* oi = get_option_info(param.c_str());
					if (oi->need_param)
					{
						if (i + 1 < argc)
						{
							save_option(param.c_str(), argv[i+1]);
							i++;
						}
						else
						{
							throw missing_option_value(param);
						}
					}
					else
					{
						save_option(param.c_str(), L"1");
					}
				}
			}
			else
			{
				// -o
				for (const wchar_t * p = arg + 1; *p; p++)
				{
					std::wstring param = std::wstring(1, *p);
					option_info* oi = get_option_info(param.c_str());
					if (!oi->need_param)
					{
						save_option(param.c_str(), L"1");
					}
					else
					{
						if (p[1])
						{
							throw missing_option_value(param);
						}
						else
						{
							if (i + 1 >= argc)
							{
								throw missing_option_value(param);
							}
							else
							{
								save_option(param.c_str(), argv[i+1]);
								i++;
								break;
							}
						}
					}
				}
			}
		}
	}

	std::wstring get_option(const wchar_t* option, const wchar_t* default_value) const
	{
		const option_info* oi = get_option_info(option);
		return oi? oi->param_value_string : default_value;
	}
	int get_option(const wchar_t* option, int default_value) const
	{
		const option_info* oi = get_option_info(option);
		if (!oi) return default_value;
		if (oi->param_type == param_type_int)
		{
			return oi->param_value_int;
		}
		else
		{
			return tp::cvt::to_int(oi->param_value_string);
		}
	}
	bool get_option(const wchar_t* option, bool default_value) const
	{
		const option_info* oi = get_option_info(option);
		if (!oi) return default_value;
		if (oi->param_type == param_type_bool)
		{
			return oi->param_value_bool;
		}
		else
		{
			return tp::cvt::to_bool(oi->param_value_string);
		}
	}

	bool option_exists(const wchar_t* opt) const
	{
		const option_info* oi = get_option_info(opt);
		return oi->option_position >= 0;
	}

	strlist_t get_targets() const
	{
		return m_targets;
	}
};

}
