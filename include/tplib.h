#pragma once

#include "format_shim.h"
#include "auto_release.h"
#include "service.h"
#include "opblock.h"
#include <stdarg.h>

#define TP_SERVICE_DEFINE_CREATE_DEPENDENCIES(...) \
    public: \
    static size_t get_create_dependencies(tp::sid_t* sids, size_t len) \
    { \
        return tp::helper::copy_dependencies(sids, len, __VA_ARGS__, -1); \
    } 
#define TP_SERVICE_DEFINE_DESTROY_DEPENDENCIES(...) \
    public: \
    static size_t get_destroy_dependencies(tp::sid_t* sids, size_t len) \
    { \
        return tp::helper::copy_dependencies(sids, len, __VA_ARGS__, -1); \
    } 

namespace tp
{
    struct helper
    {
        static void register_tp_global_services()
        {
            servicemgr& mgr = global_servicemgr();
            mgr.register_service(new service_factory_impl<opmgr>(L"opmgr", L"operation manager"));
        }
        
        //! the last variadic parameter MUST be -1
        static size_t copy_dependencies(sid_t* dst, size_t dstlen, ...)
        {
            va_list args;
            va_start(args, dstlen);
            size_t srclen = 0;
            for (sid_t sid = va_arg(args, sid_t); sid > 0; sid = va_arg(args, sid_t))
            {
                if (srclen <= dstlen) dst[srclen] = sid;
                srclen++;
            }
            va_end(args);
            return srclen;
        }
    };
}
