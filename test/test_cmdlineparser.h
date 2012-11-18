#pragma once

#include <cmdlineparser.h>
#include <unittest.h>


TPUT_DEFINE_BLOCK(L"commandline parser", L"")
{
	bool unexpected_exception = false;
	try
	{
		tp::cmdline_parser parser;

		parser.parse(L"");
		TPUT_EXPECT(parser.get_target_count() == 0, L"空命令行参数");

		parser.parse(L"\"a b\".exe c ddd \"eee ffff\" ggg:hhh");
		TPUT_EXPECT(parser.get_target_count() == 4, L"命令行中带引号");

		TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe -f"), tp::cmdline_parser::invalid_option, 0);

		bool silent = false;
		parser.register_switch(L"s", L"silent", &silent);
		parser.register_string_option(L"f", L"file");
		TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe --file"), tp::cmdline_parser::missing_option_value, 0);

		parser.parse(L"a.exe --file=123 -s");
		TPUT_EXPECT(silent, L"正确绑定开关选项");
		TPUT_EXPECT(parser.get_string_option(L"f", L"") == L"123", L"正确获取未绑定选项的值");
		TPUT_EXPECT(parser.get_switch(L"s", false) == true, L"正确获取绑定选项的值");
		TPUT_EXPECT(parser.get_int_option(L"f", 100) == 123, L"获取其他类型选项时进行正确转换");

		std::wstring t;
		parser.register_string_option(L"t", L"time", &t);
		parser.parse(L"a.exe --time=abcde -t 193");
		TPUT_EXPECT(t == L"193", L"后面的同名选项覆盖前面的选项");
		
	} catch (tp::exception&)
	{
		unexpected_exception = true;
	}

	TPUT_EXPECT(!unexpected_exception, L"不应该有未期待的exception");
}
