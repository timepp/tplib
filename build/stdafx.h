// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#pragma warning(push, 1)
#pragma warning(disable: 4350)

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <vector>
#include <list>
#include <string>
#include <map>

#pragma warning(pop)


// warning C4514: 'tp::opblock::opblock' : unreferenced inline function has been removed
#pragma warning(disable: 4514)
// warning C4710: 'std::string std::locale::name(void) const' : function not inlined
#pragma warning(disable: 4710)
// warning C4820: 'tp::TestResult' : '3' bytes padding added after data member 'tp::TestResult::success'
#pragma warning(disable: 4820)