#pragma once

#include <string.h>

namespace tp
{

struct dummy_lock
{
	void lock() {}
	void unlock() {}
};

struct critical_section_lock
{
	critical_section_lock()  
	{ 
		__try
		{
			::InitializeCriticalSection(&m_cs); 
		}
		__except (GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION ? 
               EXCEPTION_EXECUTE_HANDLER : EXCEPTION_EXECUTE_HANDLER)
		{
			memset(&m_cs, 0, sizeof(m_cs));
		}
	}
	~critical_section_lock() 
	{ 
		::DeleteCriticalSection(&m_cs); 
	}

#if (_MSC_VER >= 1700)
	_Acquires_lock_(this->m_cs)
#endif
	void lock()   
	{
		::EnterCriticalSection(&m_cs); 
	}

#if (_MSC_VER >= 1700)
	_Acquires_lock_(this->m_cs)
#endif
	void unlock() 
	{
		::LeaveCriticalSection(&m_cs); 
	}
private:
	CRITICAL_SECTION m_cs;
};

template <typename T>
class autolocker
{
public:

	explicit autolocker(T& l) : locker(l)
	{
		locker.lock();
	}

	~autolocker()
	{
		locker.unlock();
	}

private:

	autolocker& operator= (const autolocker& a);

	T& locker;
};


}
