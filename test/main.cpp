#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <tplib.h>
#include <cmdlineparser.h>
#include <exception.h>
#include <wincon.h>
#include <opblock.h>
#include <pinyin.h>
#include <unittest.h>
#include <auto_release.h>
#include <service.h>

#define TPUT_MODNAME Main

TPUT_DEFINE_BLOCK(L"auto_release", L"")
{
	std::wstring result;

	{
		TP_SCOPE_EXIT { result += L"tse1 "; };
		TPUT_EXPECT(result == L"", L"注册scope guard之后，不会立刻执行");
		TP_SCOPE_EXIT { result += L"tse2 "; };
	}
	TPUT_EXPECT(result == L"tse2 tse1 ", L"在块退出时，scope guard按照注册相反的顺序运行");

	result = L"";
	{
		TP_SCOPE_EXIT { result += L"1"; }; TP_SCOPE_EXIT { result += L"2"; };
	}
	TPUT_EXPECT(result == L"21", L"scope guard可以在代码的同一行注册");
}

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

class reader : public tp::service_impl<5>
{
};

DEFINE_SERVICE(reader, L"Reader");

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "chs");
	tp::unittest::instance().run_test();

	reader* r = tp::servicemgr::get<reader>();
	
	tp::servicemgr::instance().destroy_all_services();
	return 0;
}
