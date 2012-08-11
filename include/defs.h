#pragma once

#include <string>

#define TP_CONCAT_INNER(a,b) a##b
#define TP_CONCAT(a,b) TP_CONCAT_INNER(a,b)
#define TP_LINE_NAME(prefix) TP_CONCAT(prefix, __LINE__)
#define TP_UNIQUE_NAME(prefix) TP_CONCAT(prefix, __COUNTER__)
#define TP_WIDESTRING_INNER(x) L##x
#define TP_WIDESTRING(x) TP_WIDESTRING_INNER(x)

namespace tp
{

	//! general exception
	struct exception
	{
		std::wstring oplist;
		std::wstring message;

		exception(const std::wstring& l, const std::wstring& m) : oplist(l), message(m)
		{
		}

		exception(const exception& e) : oplist(e.oplist), message(e.message)
		{
		}
	};

}
