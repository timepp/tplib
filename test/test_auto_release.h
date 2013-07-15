#pragma once

#include <auto_release.h>
#include <unittest.h>

TPUT_DEFINE_BLOCK(L"auto_release", L"")
{
    std::wstring result;

    {
        TP_SCOPE_EXIT { result += L"tse1 "; };
        TPUT_EXPECT(result == L"", L"scope guard doesn't execute immidiately when registered");
        TP_SCOPE_EXIT { result += L"tse2 "; };
    }
    TPUT_EXPECT(result == L"tse2 tse1 ", L"scope guards are executed in the reverse order when block exit");

    result = L"";
    {
        TP_SCOPE_EXIT { result += L"1"; }; TP_SCOPE_EXIT { result += L"2"; };
    }
    TPUT_EXPECT(result == L"21", L"more scope guards can appear in same line");
}
