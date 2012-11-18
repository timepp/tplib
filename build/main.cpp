#include "stdafx.h"

#include <test_pinyin.h>
#include <test_auto_release.h>
#include <test_format_shim.h>
#include <test_cmdlineparser.h>
#include <test_service.h>

int wmain(int /*argc*/, wchar_t * /*argv*/[])
{
	setlocale(LC_ALL, "chs");
	tp::unittest::instance().run_test();

	return 0;
}
