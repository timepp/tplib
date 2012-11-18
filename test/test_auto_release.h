#pragma once

#include <auto_release.h>
#include <unittest.h>

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
