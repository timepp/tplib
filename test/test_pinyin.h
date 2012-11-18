#pragma once

#include <pinyin.h>
#include <unittest.h>


TPUT_DEFINE_BLOCK(L"pinyin", L"")
{
	tp::pinyintool pyt;
	TPUT_EXPECT(pyt.pinyin_of_string(L"��Ƿ�") == L"gui gu fen", NULL);
	TPUT_EXPECT(pyt.pinyin_of_string(L"����CPI�Ƕ���?") == L"jin nian CPI shi duo shao ?", NULL);
	TPUT_EXPECT(pyt.pinyin_of_string(L"peach���ҵ���˼") == L"peach shi tao di yi si", NULL);
	TPUT_EXPECT(pyt.fuzzy_match(L"zhong guo", L"zg"), NULL);
	TPUT_EXPECT(pyt.fuzzy_match(L"zhong guo", L"zhg"), NULL);
	TPUT_EXPECT(pyt.fuzzy_match(L"zhong guo", L"zhong"), NULL);
	TPUT_EXPECT(!pyt.fuzzy_match(L"zhong guo", L"guo"), NULL);
	
	std::wstring zh = pyt.pinyin_of_string(L"�л����񹲺͹�");
	TPUT_EXPECT(pyt.fuzzy_match(zh.c_str(), L"zhhrmghg"), NULL);
	TPUT_EXPECT(pyt.fuzzy_match(zh.c_str(), L"zhrmghg"), NULL);
}

