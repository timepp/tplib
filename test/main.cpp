#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <tplib.h>
#include <cmdlineparser.h>
#include <exception.h>
#include <wincon.h>
#include <opblock.h>
#include <pinyin.h>
#include <unittest.h>

#define TPUT_MODNAME Main

TPUT_DEFINE_BLOCK(L"format_shim", L"")
{
	TPUT_EXPECT(wcscmp(L"abc123", tp::cz(L"ab%c%d", L'c', 123)) == 0, NULL);
	TPUT_EXPECT(wcscmp(L"中国人", tp::a2w("中国人", 936)) == 0, NULL);
}

TPUT_DEFINE_BLOCK(L"pinyin", L"")
{
	tp::pinyintool pyt;
	TPUT_EXPECT(pyt.pinyin_of_string(L"龟骨粉") == L"gui gu fen", NULL);
	TPUT_EXPECT(pyt.pinyin_of_string(L"今年CPI是多少?") == L"jin nian CPI shi duo shao ?", NULL);
	TPUT_EXPECT(pyt.pinyin_of_string(L"peach是桃的意思") == L"peach shi tao di yi si", NULL);
}

TPUT_DEFINE_BLOCK(L"commandline parser", L"")
{
	bool unexpected_exception = false;
	try
	{
		tp::cmdline_parser parser;

		parser.parse(L"");
		TPUT_EXPECT(parser.get_targets().size() == 0, L"空命令行参数");

		parser.parse(L"\"a b\".exe c ddd \"eee ffff\" ggg:hhh");
		TPUT_EXPECT(parser.get_targets().size() == 4, L"命令行中带引号");

		TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe -f"), tp::cmdline_parser::invalid_option, 0);

		bool silent = false;
		parser.register_switch(L"s", L"silent", &silent);
		parser.register_option(L"f", L"file", tp::cmdline_parser::param_type_string, 0);
		TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe --file"), tp::cmdline_parser::missing_option_value, 0);

		parser.parse(L"a.exe --file=123 -s");
		TPUT_EXPECT(silent, L"正确绑定开关选项");
		TPUT_EXPECT(parser.get_option(L"f", L"") == L"123", L"正确获取未绑定选项的值");
		TPUT_EXPECT(parser.get_option(L"s", false) == true, L"正确获取绑定选项的值");
		TPUT_EXPECT(parser.get_option(L"f", 100) == 123, L"获取其他类型选项时进行正确转换");

		std::wstring t;
		parser.register_option(L"t", L"time", tp::cmdline_parser::param_type_string, &t);
		parser.parse(L"a.exe --time=abcde -t 193");
		TPUT_EXPECT(t == L"193", L"后面的同名选项覆盖前面的选项");
		
	} catch (...)
	{
		unexpected_exception = true;
	}

	TPUT_EXPECT(!unexpected_exception, L"不应该有未期待的exception");
}

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "chs");
	tp::unittest::instance().run_test();
	return 0;
}
