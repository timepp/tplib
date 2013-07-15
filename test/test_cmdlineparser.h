#pragma once

#include <cmdlineparser.h>
#include <unittest.h>

TPUT_DEFINE_BLOCK(L"commandline parser", L"")
{
    bool unexpected_exception = false;
    try
    {
        tp::cmdline_parser parser;

        parser.parse(L"");
        TPUT_EXPECT(parser.get_target_count() == 0, L"target count: empty command line");

        parser.parse(L"\"a b\".exe c ddd \"eee ffff\" ggg:hhh");
        TPUT_EXPECT(parser.get_target_count() == 4, L"target count: complex");

        TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe -f"), tp::cmdline_parser::invalid_option, L"");

        bool silent = false;
        parser.register_switch(L"s", L"silent", &silent);
        parser.register_string_option(L"f", L"file");
        TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe --file"), tp::cmdline_parser::missing_option_value, L"");

        parser.parse(L"a.exe --file=123 -s");
        TPUT_EXPECT(silent, L"switch binding");
        TPUT_EXPECT(parser.get_string_option(L"f", L"") == L"123", L"unbound option value");
        TPUT_EXPECT(parser.get_switch(L"s", false) == true, L"bound option value");
        TPUT_EXPECT(parser.get_int_option(L"f", 100) == 123, L"convertion if getting different option type");

        std::wstring t;
        parser.register_string_option(L"t", L"time", &t);
        parser.parse(L"a.exe --time=abcde -t 193");
        TPUT_EXPECT(t == L"193", L"former option with same name is overwrote");
    }
    catch (tp::cmdline_parser::parse_error&)
    {
        unexpected_exception = true;
    }

    TPUT_EXPECT(!unexpected_exception, L"no unexpected exception");
}
