#pragma once

#include <format_shim.h>
#include <unittest.h>

TPUT_DEFINE_BLOCK(L"format_shim", L"")
{
	TPUT_EXPECT(wcscmp(L"abc123", tp::cz(L"ab%c%d", L'c', 123)) == 0, NULL);
	TPUT_EXPECT(wcscmp(L"�й���", tp::a2w("�й���", 936)) == 0, NULL);
}

