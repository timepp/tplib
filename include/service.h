#pragma once

#include <list>
#include "lock.h"
#include "defs.h"

/** \file tplib service框架
 服务管理器管理系统中所有的服务。服务管理器提供统一的服务获取和销毁的入口。
 服务管理器的特点如下：
 1. 获取服务几乎无代价，正常情况下无加锁操作
 2. 服务的按需创建
 3. 可设置服务依赖，在服务的创建和销毁时，会按照和依赖兼容的顺序进行
 4. 服务创建和销毁时，循环依赖的检测
 5. 解决了系统中singleton满天飞，各自为政，生存期和依赖关系失去控制的局面。

 服务管理器使用一个静态数组保存所有服务的信息, 每个服务都对应一个整型的服务ID, 
 服务管理器使用服务ID做为下标，在服务数组中索引服务的相关信息，这使创建服务的开销降到最低。
 服务管理器要求具体服务必须事先显式设置创建和销毁时的依赖关系，这使得服务的创建和销毁变得可控，且可以应用一些检查机制。
 虽然存在一个全局服务管理器，但并不意味着服务管理器是一个singleton，可以在任何时候创建服务管理器的其它实例
 （使用场景比如：跟用户相关的服务，可以由用户服务管理器管理；在新用户登录时会创建一个新的和这个用户关联的服务管理器）

 服务管理器要求整个系统遵守以下规则：
 1. 整个系统中，除一个全局服务管理器外，没有任何复杂的全局变量或静态变量
 2. 在进程退出时，调用全局服务管理器的destroy_all_services
 3. 在调用全局服务管理器的destroy_all_services之前，确保所有线程都已经正常退出

 服务管理器要求具体的服务X遵守以下规则：
 1. 服务类X不需要、也不可以实现成singleton
 2. 服务类X必须继承tp::service_impl接口
 3. 服务类X需要用以下宏显式指定在创建时和销毁时依赖的其他服务
    * TP_SET_DEPENDENCIES(create, ...)
    * TP_SET_DEPENDENCIES(destroy, ...)
 
 限制:
 1. 服务管理器支持的服务数量有上限，这个上限通过宏TP_SERVICE_MAX控制，根据实际需要可以改变
 */

#ifndef TP_SERVICE_MAX
#define TP_SERVICE_MAX 256
#endif

namespace tp
{
	typedef int sid_t;

	/// 带引用计数的对象抽象
	struct refobj
	{
		virtual int addref() = 0;
		virtual int release() = 0;

		virtual ~refobj(){}
	};

	struct service : refobj
	{
	};

	struct service_property
	{
		sid_t sid;
		const wchar_t* name;
		const wchar_t* description;
	};

	/// 服务元信息
	struct service_factory: refobj
	{
		/** 返回该服务创建和销毁时显式依赖的服务ID
		 *  在服务创建或销毁时，尝试获取未注册的依赖服务将会失败
		 *  \param sids 用于接收依赖服务列表的缓冲区，可以为NULL
		 *  \param len  sids != NULL时有效，表示缓冲区长度
		 *  \return 销毁时，实际依赖服务的数量
		*/
		virtual size_t get_create_dependencies(sid_t* sids, size_t len) = 0;
		virtual size_t get_destroy_dependencies(sid_t* sids, size_t len) = 0;

		/** 创建service
		 *  service创建之后，引用计数为1
		 */
		virtual service* create() = 0;

		/// 服务名字和描述信息，service_factory要保证在自己的生命周期内，返回的字符串有效
		virtual service_property get_service_property() = 0;
	};

	/// service管理器
	class servicemgr
	{
	public:
		servicemgr();
		~servicemgr();

		void register_service(service_factory* factory);

		service* get_service(sid_t sid);

		template <typename T>
		T* get_service();

		void destroy_all_services();
		void clear();

	private:
		enum service_status_t
		{
			ss_none = 0,
			ss_creating_requirements,
			ss_creating,
			ss_created,
			ss_destroying_dependents,
			ss_destroying,
			ss_destroyed,
			ss_exception, // 异常状态
		};
		enum service_state_t {SS_ENABLED, SS_DISABLED};
		typedef std::list<sid_t> sidlist_t;
		struct service_info
		{
			sid_t sid;
			service_factory* factory;
			service* obj;
			service_status_t status;
			sidlist_t* creating_requirements;      // 创建时依赖哪些服务
			sidlist_t* creating_dependants;        // 被哪些服务在创建时依赖
			sidlist_t* destroying_requirements;    // 销毁时依赖哪些服务
			sidlist_t* destroying_dependants;      // 被哪些服务在销毁时依赖
			service_state_t state;
		};

		service_info m_services[TP_SERVICE_MAX];
		bool m_destroying;
		bool m_creating_service;             // 是否正在创建某个服务
		critical_section_lock m_lock;

	private:
		service_info& get_service_info(sid_t sid);
		void create_service(service_info& si);
		bool destory_service(sid_t sid);

		void create_requirements(service_info& si);
		void destroy_dependents(service_info& si);

		void enable_services(const sidlist_t* lst, service_state_t s);

		servicemgr(const servicemgr& rhs);
		servicemgr& operator=(const servicemgr& rhs);
	private:
		class service_limiter
		{
		public:
			service_limiter(servicemgr* mgr, const sidlist_t* lst) : m_mgr(mgr), m_lst(lst)
			{
				m_mgr->enable_services(NULL, SS_DISABLED);
				m_mgr->enable_services(m_lst, SS_ENABLED);
			}
			~service_limiter()
			{
				m_mgr->enable_services(NULL, SS_ENABLED);
			}
		private:
			const sidlist_t* m_lst;
			servicemgr* m_mgr;
		};
	};

	template <typename T>
	T* global_service()
	{
		return global_servicemgr().get_service<T>();
	}

	inline servicemgr& global_servicemgr()
	{
#pragma warning(push)
#pragma warning(disable:4640)
		static servicemgr mgr;
#pragma warning(pop)
		return mgr;
	}

	class refcounter
	{
	private:
		long m_ref;
	public:
		explicit refcounter(int ref = 0) : m_ref(ref) {}
		int addref()  { return ::InterlockedIncrement(&m_ref); }
		int release() { return ::InterlockedDecrement(&m_ref); }
	};

#define IMPLEMENT_HEAP_REFCOUNT \
	private: tp::refcounter m_ref; \
	virtual int addref() { return m_ref.addref(); } \
	virtual int release() { int ref = m_ref.release(); if (ref == 0) delete this; return ref; }

	template <sid_t sid>
	class service_impl : public service
	{
		refcounter m_ref;
	public:
		virtual int addref();
		virtual int release();
	public:
		enum {service_id = sid};
	};

	template <typename T>
	class service_factory_impl : public service_factory
	{
		refcounter m_ref;
		const wchar_t* m_name;
		const wchar_t* m_desc;
	public:
		virtual int addref();
		virtual int release();
		virtual size_t get_create_dependencies(sid_t* sids, size_t len);
		virtual size_t get_destroy_dependencies(sid_t* sids, size_t len);
		virtual service* create();
		virtual service_property get_service_property();

	public:
		service_factory_impl(const wchar_t* name, const wchar_t* desc);
	};

	struct service_exception : public tp::exception
	{
		sid_t service_id;
		const wchar_t* service_name;
		service_exception(sid_t sid, const wchar_t* name) : tp::exception(L"", L""), service_id(sid), service_name(name)
		{
		}
	};

#define DEF_SERVICE_EXPECTION(classname) struct classname : service_exception { explicit classname(tp::sid_t sid, const wchar_t* name = L""): service_exception(sid, name) {} }
	DEF_SERVICE_EXPECTION(service_id_conflict);
	DEF_SERVICE_EXPECTION(service_id_invalid);
	DEF_SERVICE_EXPECTION(service_destroyed);
	DEF_SERVICE_EXPECTION(service_cycle_create);
	DEF_SERVICE_EXPECTION(service_cycle_destroy);
	DEF_SERVICE_EXPECTION(service_out_of_dependency);
#undef DEF_SERVICE_EXCEPTION
}

/// impl: servicemgr
namespace tp
{
	inline servicemgr::servicemgr() : m_destroying(false)
	{
		memset(&m_services, 0, sizeof(m_services));
	}

	inline servicemgr::~servicemgr()
	{
		clear();
	}
	
	inline void servicemgr::clear()
	{
		autolocker<critical_section_lock> l(m_lock);

		destroy_all_services();
		for (sid_t sid = 0; sid < _countof(m_services); sid++)
		{
			service_info& si = get_service_info(sid);
			if (si.factory)
			{
				si.factory->release();
				si.factory = NULL;
			}
			delete si.creating_requirements; si.creating_requirements = NULL;
			delete si.creating_dependants; si.creating_dependants = NULL;
			delete si.destroying_requirements; si.destroying_requirements = NULL;
			delete si.destroying_dependants; si.destroying_dependants = NULL;
			si.state = SS_ENABLED;
			si.status = ss_none;
			si.obj = NULL;
		}
	}

	inline void servicemgr::register_service(service_factory* f)
	{
		autolocker<critical_section_lock> l(m_lock);
		service_property sp = f->get_service_property();
		service_info& si = get_service_info(sp.sid);
		if (si.factory)
		{
			throw service_id_conflict(sp.sid);
		}

		si.sid = sp.sid;
		si.status = ss_none;
		si.factory = f;
		si.factory->addref();

		sid_t sids[TP_SERVICE_MAX];
		size_t len;

		// createing depends
		len = si.factory->get_create_dependencies(sids, _countof(sids));
		si.creating_requirements = new sidlist_t(sids, sids + len);

		// destroying depends
		len = si.factory->get_destroy_dependencies(sids, _countof(sids));
		si.destroying_requirements = new sidlist_t(sids, sids + len);
		for (size_t i = 0; i < len; i++)
		{
			service_info& si2 = get_service_info(sids[i]);
			if (!si2.destroying_dependants)
			{
				si2.destroying_dependants = new sidlist_t;
			}
			si2.destroying_dependants->push_back(si.sid);
		}
	}

	inline void servicemgr::destroy_all_services()
	{
		autolocker<critical_section_lock> l(m_lock);

		m_destroying = true;

		// 在销毁一个服务的过程中，有可能创建出新的服务，所以，整个销毁服务的过程，需要多次扫描
		// 直到某次扫描过程中，已经没有服务被销毁为止
		bool need_rescan = true;
		while (need_rescan)
		{
			need_rescan = false;
			for (size_t i = 0; i < _countof(m_services); i++)
			{
				bool destroyed = destory_service(static_cast<sid_t>(i));
				if (destroyed) need_rescan = true;
			}
		}

		m_destroying = false;
	}

	inline service* servicemgr::get_service(sid_t sid)
	{
		service_info& si = get_service_info(sid);
		if (si.state == SS_DISABLED) throw service_out_of_dependency(sid);

		switch (si.status)
		{
		case ss_none:
			create_service(si);
			return si.obj;
		case ss_creating:
		case ss_creating_requirements:
			throw service_cycle_create(sid);
		case ss_created:
		case ss_destroying_dependents:
			return si.obj;
		case ss_destroying:
			throw service_cycle_destroy(sid);
		case ss_destroyed:
			throw service_destroyed(sid);
		case ss_exception:
			return NULL; // 已经发生过异常，不再抛异常，而是直接返回NULL
		default:
			throw std::runtime_error("unknown service status");
		}
	}

	template <typename T>
	T* servicemgr::get_service()
	{
		service* s = get_service(T::service_id);
		return static_cast<T*>(s);
	}

	inline servicemgr::service_info& servicemgr::get_service_info(sid_t sid)
	{
		if (sid >= _countof(m_services))
		{
			throw service_id_invalid(sid);
		}

		return m_services[sid];
	}

	inline void servicemgr::create_requirements(service_info& si)
	{
		if (si.creating_requirements)
		{
			for (sidlist_t::const_iterator it = si.creating_requirements->begin(); it != si.creating_requirements->end(); ++it)
			{
				service_info& si = get_service_info(*it);
				create_service(si);
			}
		}
	}

	inline void servicemgr::destroy_dependents(service_info& si)
	{
		if (si.destroying_dependants)
		{
			for (sidlist_t::const_iterator it = si.destroying_dependants->begin(); it != si.destroying_dependants->end(); ++it)
			{
				destory_service(*it);
			}
		}
	}

	inline bool servicemgr::destory_service(sid_t sid)
	{
		autolocker<critical_section_lock> l(m_lock);
		service_info& si = get_service_info(sid);
		switch (si.status)
		{
		case ss_none:
		case ss_exception:
		case ss_destroyed: return false;
		case ss_destroying_dependents:
		case ss_destroying: throw service_cycle_destroy(si.sid);
		case ss_creating_requirements:
		case ss_creating: throw service_exception(si.sid, L"");
		case ss_created: break;
		}

		if (!si.factory) throw service_id_invalid(si.sid);

		try
		{
			si.status = ss_destroying_dependents;

			destroy_dependents(si);

			si.status = ss_destroying;

			service_limiter lmt(this, si.destroying_requirements);
			si.obj->release();
			si.obj = NULL;

			si.status = ss_destroyed;
		}
		catch (...)
		{
			si.status = ss_exception;
			throw;
		}

		return true;
	}

	inline void servicemgr::create_service(service_info& si)
	{
		autolocker<critical_section_lock> l(m_lock);
		switch (si.status)
		{
		case ss_created:
		case ss_exception: return;
		case ss_creating_requirements:
		case ss_creating: throw service_cycle_create(si.sid);
		case ss_destroying_dependents:
		case ss_destroying:
		case ss_destroyed: throw service_destroyed(si.sid);
		case ss_none: break;
		}

		if (!si.factory) throw service_id_invalid(si.sid);

		try
		{
			si.status = ss_creating_requirements;

			create_requirements(si);

			si.status = ss_creating;

			service_limiter lmt(this, si.creating_requirements);
			si.obj = si.factory->create();

			si.status = ss_created;
		}
		catch (...)
		{
			si.status = ss_exception;
			throw;
		}
	}

	inline void servicemgr::enable_services(const sidlist_t* lst, servicemgr::service_state_t s)
	{
		if (!lst)
		{
			for (size_t i = 0; i < _countof(m_services); i++)
			{
				m_services[i].state = s;
			}
		}
		else
		{
			for (sidlist_t::const_iterator it = lst->begin(); it != lst->end(); ++it)
			{
				m_services[*it].state = s;
			}
		}
	}
}

/// impl: service_impl
namespace tp
{
	template <sid_t sid>
	inline int service_impl<sid>::addref()
	{
		return m_ref.addref();
	}
	template <sid_t sid>
	inline int service_impl<sid>::release()
	{
		int ref = m_ref.release();
		if (ref == 0) delete this;
		return ref;
	}
}

/// impl: service_factory_impl
namespace tp
{
	template <typename T>
	service_factory_impl<T>::service_factory_impl(const wchar_t* name, const wchar_t* desc)
		: m_name(name), m_desc(desc)
	{
	}

	template <typename T>
	inline int service_factory_impl<T>::addref()
	{
		return m_ref.addref();
	}

	template <typename T>
	inline int service_factory_impl<T>::release()
	{
		int ref = m_ref.release();
		if (ref == 0) delete this;
		return ref;
	}

	template <typename T>
	inline service_property service_factory_impl<T>::get_service_property()
	{
		service_property sp;
		sp.sid = T::service_id;
		sp.name = m_name;
		sp.description = m_desc;
		return sp;
	}

	template <typename T>
	inline size_t service_factory_impl<T>::get_create_dependencies(sid_t* sids, size_t len)
	{
		return T::get_create_dependencies(sids, len);
	}

	template <typename T>
	inline size_t service_factory_impl<T>::get_destroy_dependencies(sid_t* sids, size_t len)
	{
		return T::get_destroy_dependencies(sids, len);
	}

	template <typename T>
	inline service* service_factory_impl<T>::create()
	{
		service* s = new T;
		s->addref();
		return s;
	}
};

namespace tp
{
	template <typename T>
	struct scoped_ref_ptr
	{
		T* m_ptr;

		scoped_ref_ptr(T* p = NULL) : m_ptr(p)
		{
			if (m_ptr) m_ptr->addref();
		}
		scoped_ref_ptr(const scoped_ref_ptr& rhs) : m_ptr(rhs.m_ptr)
		{
			if (m_ptr) m_ptr->addref();
		}
		scoped_ref_ptr& operator=(T* p)
		{
			if (p) p->addref();
			if (m_ptr) m_ptr->release();
			m_ptr = p;
			return *this;
		}
		scoped_ref_ptr& operator=(const scoped_ref_ptr& rhs)
		{
			return operator=(rhs.m_ptr);
		}

		~scoped_ref_ptr()
		{
			if (m_ptr) m_ptr->release();
			m_ptr = NULL;
		}
		T* operator->()
		{
			return m_ptr;
		}
	};

	template <typename T>
	struct naked_ptr
	{
		T* m_ptr;

		naked_ptr(T* p = NULL) : m_ptr(p) {}
		T* operator->() { return m_ptr; }
	};
}

#ifdef TP_SERVICE_ENABLE_REFCOUNT
	#define service_ptr tp::scoped_ref_ptr
#else
	#define service_ptr tp::naked_ptr
#endif

namespace tp
{
	const sid_t SID_OPMGR = TP_SERVICE_MAX - 1;
}


#define TP_DEFINE_GLOBAL_SERVICE3(classname, service_desc, defid) \
	template <typename T> struct ServiceInitHelper_##defid { \
		ServiceInitHelper_##defid() { \
			tp::global_servicemgr().register_service(new tp::service_factory_impl<classname>(TP_WIDESTRING(#classname), service_desc)); \
		} \
		static ServiceInitHelper_##defid<int> s_initializer; \
	}; \
	ServiceInitHelper_##defid<int> ServiceInitHelper_##defid<int>::s_initializer;
#define TP_DEFINE_GLOBAL_SERVICE2(classname, service_desc, defid) TP_DEFINE_GLOBAL_SERVICE3(classname, service_desc, defid)
#define TP_DEFINE_GLOBAL_SERVICE(classname, service_desc) TP_DEFINE_GLOBAL_SERVICE2(classname, service_desc, __COUNTER__)

#define TP_SET_DEPENDENCIES(phase, ...) \
	public:static size_t get_##phase##_dependencies(tp::sid_t* sids, size_t len) { \
		tp::sid_t dst[] = {0, __VA_ARGS__}; \
		size_t dstlen = _countof(dst) - 1; \
		if (len >= dstlen) { for (size_t i = 0; i < dstlen; i++) sids[i] = dst[i+1]; } \
		return dstlen; \
	}
