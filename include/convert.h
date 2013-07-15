#pragma once

#include <string>
#include <stdlib.h>

namespace tp
{

struct cvt
{
    static int to_int(const wchar_t* s)
    {
        return _wtoi(s);
    }
    static int to_int(const std::wstring& s)
    {
        return to_int(s.c_str());
    }

    static bool to_bool(const wchar_t* s)
    {
        return to_int(s)? true : false;
    }
    static bool to_bool(const std::wstring& s)
    {
        return to_bool(s.c_str());
    }
};



}