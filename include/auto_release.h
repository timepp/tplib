#pragma once

#include "defs.h"

/** \file auto_release.h

 auto execute statements when leaving current scope

 @code
   ON_LEAVE(printf("block exit"));
   ...
   FILE* fp = fopen("a.txt");
   ON_LEAVE_1(fclose(fp), FILE*, fp);

   // the following code only available after VS2012
   TP_SCOPE_EXIT { fclose(fp); };
 @endcode

 @sa
   - scope guard in D language: http://www.digitalmars.com/d/2.0/statement.html#ScopeGuardStatement
   - boost scope exit: http://www.google.com/#q=boost+scope+exit
 */


//! execute <b>statement</b> on scope exit
#define ON_LEAVE(statement) \
    struct TP_LINE_NAME(ols_) { \
        ~TP_LINE_NAME(ols_)() { statement; } \
    } TP_LINE_NAME(olv_);

//! execute <b>statement</b> on scope exit, with one local variable <b>var</b> of type <b>type</b>
#define ON_LEAVE_1(statement, type, var) \
    struct TP_LINE_NAME(ols_) { \
        type var; \
        TP_LINE_NAME(ols_)(type v): var(v) {} \
        ~TP_LINE_NAME(ols_)() { statement; } \
    } TP_LINE_NAME(olv_)(var);

//! execute <b>statement</b> on scope exit, with two local variables
#define ON_LEAVE_2(statement, type1, var1, type2, var2) \
    struct TP_LINE_NAME(ols_) { \
        type1 var1; type2 var2; \
        TP_LINE_NAME(ols_)(type1 v1, type2 v2): var1(v1), var2(v2) {} \
        ~TP_LINE_NAME(ols_)() { statement; } \
    } TP_LINE_NAME(olv_)(var1, var2);

//! execute <b>statement</b> on scope exit, with 3 local variables
#define ON_LEAVE_3(statement, type1, var1, type2, var2, type3, var3) \
    struct TP_LINE_NAME(ols_) { \
        type1 var1; type2 var2; type3 var3; \
        TP_LINE_NAME(ols_)(type1 v1, type2 v2, type3 v3): var1(v1), var2(v2), var3(v3) {} \
        ~TP_LINE_NAME(ols_)() { statement; } \
    } TP_LINE_NAME(olv_)(var1, var2, var3);

//! execute <b>statement</b> on scope exit, with 4 local variables
#define ON_LEAVE_4(statement, type1, var1, type2, var2, type3, var3, type4, var4) \
    struct TP_LINE_NAME(ols_) { \
        type1 var1; type2 var2; type3 var3; type4 var4; \
        TP_LINE_NAME(ols_)(type1 v1, type2 v2, type3 v3, type4 v4): var1(v1), var2(v2), var3(v3), var4(v4) {} \
        ~TP_LINE_NAME(ols_)() { statement; } \
    } TP_LINE_NAME(olv_)(var1, var2, var3, var4);


#if (_MSC_VER >= 1700)
#include <functional>

namespace tp
{
    //! tplib inner class used by TP_SCOPE_EXIT
    class scope_exit_t
    {
        using func_type = std::function<void()>;

    public:
        // To make scope_exit_t can do direct-initialization with lambda
        // Otherwise we cannot write like this: scope_exit_t var = [&]() {...}
        // since this copy-initialization requires 2 implict conversion which is not allowed
        // https://stackoverflow.com/questions/28875231/initialization-and-lambda-type-argument

        template<typename T>
        scope_exit_t(T&& t) : func(std::move(t)) 
        { 
        }

        ~scope_exit_t() 
        { 
            func(); 
        }

        const func_type func;
    };
}

/**
 scope guard using C++0x syntax
 */
#define TP_SCOPE_EXIT tp::scope_exit_t TP_UNIQUE_NAME(tpse_) = [&]()
//#define TP_SCOPE_EXIT

#endif
