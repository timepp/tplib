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
	TPUT_EXPECT(pyt.pinyin_of_string(L"��Ƿ�") == L"gui gu fen");
	TPUT_EXPECT(pyt.pinyin_of_string(L"����CPI�Ƕ���?") == L"jin nian CPI shi duo shao ?");
	TPUT_EXPECT(pyt.pinyin_of_string(L"peach���ҵ���˼") == L"peach shi tao di yi si");
}

TPUT_DEFINE_BLOCK(L"commandline parser", L"")
{
	tp::cmdline_parser parser;

	parser.parse(L"");
	TPUT_EXPECT_WITH_MSG(parser.get_targets().size() == 0, L"�������в���");

	parser.parse(L"ccc.exe /x \"b b\" --aaaaa -- \"ffff gggg hhhh\" 'gggg'");
	TPUT_EXPECT(parser.get_targets().size() == 6);
}

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "chs");
	tp::unittest::instance().run_test();
	return 0;
}
