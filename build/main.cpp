#include "stdafx.h"

#include <tplib.h>
#include <test_pinyin.h>
#include <test_auto_release.h>
#include <test_format_shim.h>
#include <test_cmdlineparser.h>
#include <test_service.h>

#include <vector>

int wmain(int /*argc*/, wchar_t * /*argv*/[])
{
	setlocale(LC_ALL, "chs");
	tp::helper::register_tp_global_services();
	tp::ConsoleTestOutput o;
	tp::unittest::instance().set_test_output(&o);
	tp::unittest::instance().run_test(L"auto_release");

	return 0;
}
