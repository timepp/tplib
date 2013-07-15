#pragma once

#include <pinyin.h>
#include <unittest.h>

TPUT_DEFINE_BLOCK(L"pinyin", L"")
{
    tp::pinyintool pyt;
    TPUT_EXPECT(pyt.pinyin_of_string(L"龟骨粉") == L"gui gu fen", NULL);
    TPUT_EXPECT(pyt.pinyin_of_string(L"今年CPI是多少?") == L"jin nian CPI shi duo shao ?", NULL);
    TPUT_EXPECT(pyt.pinyin_of_string(L"peach是桃的意思") == L"peach shi tao di yi si", NULL);
    TPUT_EXPECT(pyt.fuzzy_match(L"zhong guo", L"zg"), NULL);
    TPUT_EXPECT(pyt.fuzzy_match(L"zhong guo", L"zhg"), NULL);
    TPUT_EXPECT(pyt.fuzzy_match(L"zhong guo", L"zhong"), NULL);
    TPUT_EXPECT(!pyt.fuzzy_match(L"zhong guo", L"guo"), NULL);

    TPUT_EXPECT(pyt.fuzzy_match(L"zhong hua ren min gong he guo", L"zhhrmghg"), NULL);
    TPUT_EXPECT(pyt.fuzzy_match(L"zhong hua ren min gong he guo", L"z"), NULL);
    TPUT_EXPECT(pyt.fuzzy_match(L"zhong hua ren min gong he guo", L"zh"), NULL);
    TPUT_EXPECT(pyt.fuzzy_match(L"zhong hua ren min gong he guo", L"zhr"), NULL);
    TPUT_EXPECT(pyt.fuzzy_match(L"zhong hua ren min gong he guo", L"zhhr"), NULL);
    TPUT_EXPECT(pyt.fuzzy_match(L"zhong hua ren min gong he guo", L"zhuarmg"), NULL);
    TPUT_EXPECT(!pyt.fuzzy_match(L"zhong hua ren min gong he guo", L"zhohua"), NULL);
    TPUT_EXPECT(pyt.fuzzy_match(L"zhong hua ren min gong he guo", L"zhhrmghguo"), NULL);
}
