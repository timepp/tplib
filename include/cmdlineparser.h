#pragma once

#include <string>
#include <list>
#include "./auto_release.h"
#include "./exception.h"
#include "./format_shim.h"


namespace tp
{

class cmdline_parser
{
public:
	typedef std::list<std::wstring> strlist_t;
	enum param_type_t {param_type_int, param_type_bool, param_type_string};

private:
	struct option_info
	{
		const wchar_t* short_name;
		const wchar_t* long_name;
		const wchar_t* desc;
		bool need_param;
		param_type_t param_type;
		void* value_receiver;

		int option_position;                   // 选项在命令行出现的顺序，-1表示选项不存在
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
		throw_custom_error(cz(L"无效的选项[%s]", opt));
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
		if (oi->param_type == param_type_string)
		{
			oi->param_value_string = buffer;
			if (oi->value_receiver)
			{
				*(std::wstring*)oi->value_receiver = oi->param_value_string;
			}
		}
		else if (oi->param_type == param_type_int)
		{
			oi->param_value_int = _wtoi(buffer);
			if (oi->value_receiver)
			{
				*(int*)oi->value_receiver = oi->param_value_int;
			}
		}
		else if (oi->param_type == param_type_bool)
		{
			oi->param_value_bool = _wtoi(buffer)? true : false;
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

	//! parse from command line
	//! A sample command-line:   
	//! aaa.exe --value=cc -hls -t "c d e" -- -fffc.txt
	void parse(const wchar_t* cmd_line)
	{
		/** windows下对命令行的处理：参数中间有空格时，可以用引号引起来。这样就有一个如何表示引号的问题。
		 *  使用\"来表示"。\如果在引号之前，表示那是一个普通引号。\在其它位置保持其本意
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
			argv[i++] = it->c_str();
		}

		parse(argc, argv);
	}

	//! parse from main()'s argc and argv
	void parse(size_t argc, const wchar_t* const * argv)
	{
		bool in_targets = false;
		for (size_t i = 1; i < argc; i++)
		{
			const wchar_t* arg = argv[i];

			if (arg[0] != '-' || in_targets)
			{
				m_targets.push_back(arg);
				continue;
			}
			if (arg[1] == '-' && !arg[2]) // -- 后是targets
			{
				in_targets = true;
				continue;
			}

			if (arg[1] == '-')
			{
				// 两个减号
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
							throw_custom_error(cz(L"选项[%s]缺少参数", param.c_str()));
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
				// 一个减号
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
							throw_custom_error(cz(L"选项[%s]缺少参数", param.c_str()));
						}
						else
						{
							if (i + 1 >= argc)
							{
								throw_custom_error(cz(L"选项[%s]缺少参数", param.c_str()));
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
		return oi? oi->param_value_int : default_value;
	}
	bool get_option(const wchar_t* option, bool default_value) const
	{
		const option_info* oi = get_option_info(option);
		return oi? oi->param_value_bool : default_value;
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
