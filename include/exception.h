#ifndef TP_EXCEPTION_H_INCLUDED
#define TP_EXCEPTION_H_INCLUDED

#include "defs.h"
#include "format_shim.h"
#include "opblock.h"

#ifdef NE
#pragma warning("conflict: NE already defined")
#else
#define NE(statment1, statment2) try { statment1; } catch (...) { statment2; }
#endif

namespace tp
{
	//! general exception
	struct exception_with_oplist : public tp::exception
	{
		explicit exception_with_oplist(const wchar_t* msg = L"") : tp::exception(CURRENT_OPLIST(), msg)
		{
		}
	};

	struct win_exception : public exception_with_oplist
	{
		DWORD errorcode;
		win_exception(DWORD e) : errorcode(e)
		{
			message = tp::edwin(errorcode);
		}
	};
	struct com_exception : public exception_with_oplist
	{
		HRESULT ret;
		com_exception(HRESULT e) : ret(e)
		{
			message = tp::edcom(ret);
		}
	};
	struct dos_exception : public exception_with_oplist
	{
		int errorcode;
		dos_exception(int e) : errorcode(e)
		{
			message = tp::edstd(errorcode);
		}
	};

	inline void throw_winerr_when(bool cond)
	{
		if (cond) 
		{
			throw tp::win_exception(GetLastError());
		}
	}
	inline void throw_stderr_when(bool cond)
	{
		if (cond)
		{
			throw tp::dos_exception(errno);
		}
	}
	inline void throw_when_fail(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw tp::com_exception(hr);
		}
	}
	inline void throw_when(bool cond, const wchar_t* msg)
	{
		if (cond)
		{
			throw tp::exception_with_oplist(msg);
		}
	}
	
}

#endif