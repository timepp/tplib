#pragma once

#include <format_shim.h>
#include <unittest.h>

TPUT_DEFINE_BLOCK(L"format_shim", L"")
{
	TPUT_EXPECT(wcscmp(L"abc123", tp::cz(L"ab%c%d", L'c', 123)) == 0, NULL);
	TPUT_EXPECT(wcscmp(L"中国人", tp::a2w("中国人", 936)) == 0, NULL);
}

