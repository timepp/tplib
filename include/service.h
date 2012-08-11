#pragma once

#include <list>
#include "lock.h"
#include "defs.h"

/** \file tplib service框架
 服务管理器以singleton模式实现，管理系统中所有的服务。服务管理器提供如下功能：
 1. 服务的统一获取入口
 2. 服务的按需创建
 3. 服务的销毁，可设置服务销毁时的依赖关系
 4. 服务创建和销毁时，循环依赖的检测

 服务管理器要求系统遵守以下规则：
 1. 整个系统中，除服务管理器外，没有任何复杂的全局变量或静态变量
 2. 在进程退出之前，所有线程退出之后，调用服务管理器的destroy_all_services

 在服务管理器中，保存了所有服务的信息，包括服务如何创建和销毁、服务销毁时的依赖关系、服务指针和服务状态。
 服务管理器通过服务ID来获取某个服务，为了高效访问，服务ID定义为整型，服务管理器通过把服务ID做为下标，可以快速的索引到具体的服务。

 服务管理器要求具体的服务X遵守以下规则：
 1. 服务类X不需要、也不可以实现成singleton
 2. 服务类X必须继承tp::service_impl接口
 3. 服务类X必须在合适的时机调用add_service_info注册给服务管理器，这一般通过在类定义之后使用DEFINE_SERVICE实现
 4. 如果服务X在销毁时需要用到服务Y，需要调用服务管理器的add_destroy_dependency增加销毁时的依赖关系
 
 限制:
 1. 服务管理器支持的服务数量有上限，这个上限通过宏TP_SERVICE_MAX控制，根据实际需要可以改变
 */

#ifndef TP_SERVICE_MAX
#define TP_SERVICE_MAX  256
#endif

namespace tp
{
	struct service
	{
		virtual ~service() {}
	};

	struct service_creator
	{
		virtual ~service_creator() {}

		virtual service* create_service() = 0;
		virtual void destroy_service(service* s) = 0;
	};
	
	typedef int service_id_t;

	class servicemgr
	{
	public:
		struct service_exception : public tp::exception
		{
			service_id_t service_id;
			const wchar_t* service_name;
			service_exception(service_id_t sid, const wchar_t* name) : tp::exception(L"", L""), service_id(sid), service_name(name)
			{
			}
		};
		struct service_id_conflict : public service_exception
		{
			service_id_conflict(service_id_t sid, const wchar_t* name) : service_exception(sid, name)
			{
			}
		};
		struct service_destroyed : public service_exception
		{
			service_destroyed(service_id_t sid, const wchar_t* name) : service_exception(sid, name)
			{
			}
		};
		struct service_id_invalid : public service_exception
		{
			service_id_invalid(service_id_t sid, const wchar_t* name) : service_exception(sid, name)
			{
			}
		};

	public:
		static servicemgr& instance()
		{
			__pragma(warning(push))
			__pragma(warning(disable:4640))
			static servicemgr mgr;
			__pragma(warning(pop))
			return mgr;
		}

		void add_service_info(service_id_t sid, service_creator* creator, const wchar_t* service_name, const wchar_t* desc)
		{
			autolocker<critical_section_lock> l(m_lock);

			service_info& si = get_service_info(sid);
			if (si.creator || si.obj)
			{
				throw service_id_conflict(sid, service_name);
			}

			si.name = service_name;
			si.desc = desc;
			si.creator = creator;
		}

		/// before calling this function,
		/// ensure all other threads are terminated
		void destroy_all_services()
		{
			autolocker<critical_section_lock> l(m_lock);

			m_destroying = true;

			for (size_t i = 0; i < _countof(m_services); i++)
			{
				destory_service(i);
			}

			m_destroying = false;
		}

		void add_destroy_dependency(service_id_t sid, service_id_t dependent_sid)
		{
			autolocker<critical_section_lock> l(m_lock);

			service_info& si = get_service_info(dependent_sid);
			if (!si.m_dependants)
			{
				si.m_dependants = new std::list<service_id_t>;
			}
			si.m_dependants->push_back(sid);
		}

		template <typename T>
		static T* get()
		{
			service_info& si = servicemgr::instance().get_service_info(T::service_id);
			if (si.status == ss_created)
			{
				return static_cast<T*>(si.obj);
			}
			if (si.status == ss_destroyed || si.status == ss_destroying)
			{
				throw service_destroyed(T::service_id, si.name);
			}
			if (si.status == ss_none)
			{
				servicemgr::instance().create_service(si);
			}

			return static_cast<T*>(si.obj);
		};

	private:
		enum service_status_t
		{
			ss_none,
			ss_created,
			ss_destroying,
			ss_destroyed
		};
		struct service_info
		{
			const wchar_t* name;
			const wchar_t* desc;
			service* obj;
			service_creator* creator;
			service_status_t status;
			std::list<service_id_t>* m_dependants;
			service_info() : name(0), obj(0), creator(0), status(ss_none), m_dependants(0)
			{
			}
		};

		service_info m_services[TP_SERVICE_MAX];
		bool m_destroying;
		critical_section_lock m_lock;

	private:
		service_info& get_service_info(service_id_t sid)
		{
			size_t index = static_cast<size_t>(sid);
			if (index >= _countof(m_services))
			{
				throw service_id_invalid(sid, L"");
			}

			return m_services[index];
		}

		void destory_service(service_id_t sid)
		{
			service_info& si = get_service_info(sid);
			if (si.status != ss_created) return;

			si.status = ss_destroying;

			if (si.m_dependants)
			{
				for (std::list<service_id_t>::const_iterator it = si.m_dependants->begin(); it != si.m_dependants->end(); ++it)
				{
					destory_service(*it);
				}

				delete si.m_dependants;
				si.m_dependants = NULL;
			}

			si.creator->destroy_service(si.obj);
			si.status = ss_destroyed;
			si.obj = NULL;
		}

		void create_service(service_info& si)
		{
			autolocker<critical_section_lock> l(m_lock);
			if (si.status == ss_none)
			{
				si.obj = si.creator->create_service();
				si.status = ss_created;
			}
		}

		servicemgr() : m_destroying(false)
		{
		}

		servicemgr(const servicemgr& rhs);
		servicemgr& operator=(const servicemgr& rhs);
	};

	template <service_id_t sid>
	class service_impl : public service
	{
	public:
		enum {service_id = sid};
	};

	template <typename T>
	struct service_creator_impl : public service_creator
	{
		service* create_service()
		{
			return new T;
		}
		void destroy_service(service* s)
		{
			delete s;
		}
	};
	
}

/// service_id reserved by tplib
namespace tp
{
	const service_id_t SID_OPMGR = 0xF0;
}

#define DEFINE_SERVICE(classname, service_desc) \
	template <typename T> struct ServiceInitHelper_##classname { \
		tp::service_creator_impl<classname> m_creator; \
		ServiceInitHelper_##classname() { \
			tp::servicemgr::instance().add_service_info(classname::service_id, &m_creator, TP_WIDESTRING(#classname), service_desc); \
		} \
		static ServiceInitHelper_##classname<int> s_initializer; \
	}; \
	ServiceInitHelper_##classname<int> ServiceInitHelper_##classname<int>::s_initializer;
