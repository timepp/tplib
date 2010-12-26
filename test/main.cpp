#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <tplib.h>
#include <cmdlineparser.h>
#include <exception.h>
#include <wincon.h>
#include <opblock.h>

int wmain(int argc, wchar_t *argv[])
{
	tp::sc::log_default_console_config();
	tp::log(L"main enter");
	ON_LEAVE(tp::log(L"main leave"));

	tp::cmdline_parser parser;
	parser.parse(argc, argv);

	tp::log(tp::cz(L"param count: %d, target count: %d", argc, parser.get_target_count()));

	return 0;
}
