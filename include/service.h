#pragma once

#include <list>

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

	typedef int service_id_t;

	class servicemgr
	{
	public:
		static servicemgr& instance()
		{
			__pragma(warning(push))
			__pragma(warning(disable:4640))
			static servicemgr mgr;
			__pragma(warning(pop))
			return mgr;
		}

		void add_service_info(service_id_t sid, service_creator* creator, const wchar_t* service_name)
		{
			service_info& si = get_service_info(sid);
			if (si.creator || si.obj)
			{
				// throw id-conflict
			}

			si.name = service_name;
			si.creator = creator;
		}

		/// before calling this function,
		/// ensure all other threads are terminated
		void destroy_all_services()
		{
			m_destroying = true;

			for (size_t i = 0; i < _countof(m_services); i++)
			{
				destory_service(i);
			}

			for (size_t i = 0; i < _countof(m_services); i++)
			{
				service_info& si = m_services[i];
				if (si.obj && si.status != ss_destroyed)
				{
					// throw service-not-destroyed
				}
			}

			m_destroying = false;
		}

		void add_destroying_dependency(service_id_t sid, service_id_t dependent_sid)
		{
			service_info& si = get_service_info(dependent_sid);
			si.m_dependants.push_back(sid);
		}

		template <typename T>
		static T* get()
		{
			service_info& si = servicemgr::instance().get_service_info(T::service_id);
			if (si.status == ss_destroyed)
			{
				// throw
			}
			if (!si.obj)
			{
				// TODO: lock
				si.obj = si.creator->create_service();
				si.status = ss_created;
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
			service* obj;
			service_creator* creator;
			service_status_t status;
			std::list<service_id_t> m_dependants;
			service_info() : name(0), obj(0), creator(0), status(ss_none)
			{
			}
		};
		service_info m_services[256];

		bool m_destroying;

	private:
		service_info& get_service_info(service_id_t sid)
		{
			size_t index = static_cast<size_t>(sid);
			if (index >= _countof(m_services))
			{
				// throw out-of-index
			}

			return m_services[index];
		}

		void destory_service(service_id_t sid)
		{
			service_info& si = get_service_info(sid);
			if (si.status != ss_created) return;

			si.status = ss_destroying;
			for (std::list<service_id_t>::const_iterator it = si.m_dependants.begin(); it != si.m_dependants.end(); ++it)
			{
				destory_service(*it);
			}

			si.m_dependants.clear();
			si.creator->destroy_service(si.obj);
			si.status = ss_destroyed;
			si.obj = NULL;
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
	
	// TP ÄÚÖÃµÄservice_id
	const service_id_t SID_OPMGR = 0xF0;
}

#define DEFINE_SERVICE(classname, name) \
	template <typename T> struct ServiceInitHelper_##classname { \
		tp::service_creator_impl<classname> m_creator; \
		ServiceInitHelper_##classname() { tp::servicemgr::instance().add_service_info(classname::service_id, &m_creator, name); } \
		static ServiceInitHelper_##classname<int> s_initializer; \
	}; \
	ServiceInitHelper_##classname<int> ServiceInitHelper_##classname<int>::s_initializer;
