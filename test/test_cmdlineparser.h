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
		TPUT_EXPECT(parser.get_target_count() == 0, L"�������в���");

		parser.parse(L"\"a b\".exe c ddd \"eee ffff\" ggg:hhh");
		TPUT_EXPECT(parser.get_target_count() == 4, L"�������д�����");

		TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe -f"), tp::cmdline_parser::invalid_option, 0);

		bool silent = false;
		parser.register_switch(L"s", L"silent", &silent);
		parser.register_string_option(L"f", L"file");
		TPUT_EXPECT_EXCEPTION(parser.parse(L"a.exe --file"), tp::cmdline_parser::missing_option_value, 0);

		parser.parse(L"a.exe --file=123 -s");
		TPUT_EXPECT(silent, L"��ȷ�󶨿���ѡ��");
		TPUT_EXPECT(parser.get_string_option(L"f", L"") == L"123", L"��ȷ��ȡδ��ѡ���ֵ");
		TPUT_EXPECT(parser.get_switch(L"s", false) == true, L"��ȷ��ȡ��ѡ���ֵ");
		TPUT_EXPECT(parser.get_int_option(L"f", 100) == 123, L"��ȡ��������ѡ��ʱ������ȷת��");

		std::wstring t;
		parser.register_string_option(L"t", L"time", &t);
		parser.parse(L"a.exe --time=abcde -t 193");
		TPUT_EXPECT(t == L"193", L"�����ͬ��ѡ���ǰ���ѡ��");
		
	} catch (tp::exception&)
	{
		unexpected_exception = true;
	}

	TPUT_EXPECT(!unexpected_exception, L"��Ӧ����δ�ڴ���exception");
}
