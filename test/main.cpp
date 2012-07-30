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

TPUT_DEFINE_BLOCK(L"pinyin", L"")
{
	tp::pinyintool pyt;
	TPUT_EXPECT(pyt.pinyin_of_string(L"��Ƿ�") == L"gui gu fen", NULL);
	TPUT_EXPECT(pyt.pinyin_of_string(L"����CPI�Ƕ���?") == L"jin nian CPI shi duo shao ?", NULL);
	TPUT_EXPECT(pyt.pinyin_of_string(L"peach���ҵ���˼") == L"peach shi tao di yi si", NULL);
}

TPUT_DEFINE_BLOCK(L"commandline parser", L"")
{
	bool unexpected_exception = false;
	try
	{
		tp::cmdline_parser parser;

		parser.parse(L"");
		TPUT_EXPECT(parser.get_targets().size() == 0, L"�������в���");

		parser.parse(L"\"a b\".exe c ddd \"eee ffff\" ggg:hhh");
		TPUT_EXPECT(parser.get_targets().size() == 4, L"�������д�����");

		TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe -f"), tp::cmdline_parser::invalid_option, 0);

		bool silent = false;
		parser.register_switch(L"s", L"silent", &silent);
		parser.register_option(L"f", L"file", tp::cmdline_parser::param_type_string, 0);
		TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe --file"), tp::cmdline_parser::missing_option_value, 0);

		parser.parse(L"a.exe --file=123 -s");
		TPUT_EXPECT(silent, L"��ȷ�󶨿���ѡ��");
		TPUT_EXPECT(parser.get_option(L"f", L"") == L"123", L"��ȷ��ȡδ��ѡ���ֵ");
		TPUT_EXPECT(parser.get_option(L"s", false) == true, L"��ȷ��ȡ��ѡ���ֵ");
		TPUT_EXPECT(parser.get_option(L"f", 100) == 123, L"��ȡ��������ѡ��ʱ������ȷת��");
		
	} catch (...)
	{
		unexpected_exception = true;
	}

	TPUT_EXPECT(!unexpected_exception, L"��Ӧ����δ�ڴ���exception");
}

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "chs");
	tp::unittest::instance().run_test();
	return 0;
}
