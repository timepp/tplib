#ifndef TP_OPBLOCK_H_INCLUDED
#define TP_OPBLOCK_H_INCLUDED

#include <map>
#include <list>
#include <string>
#include "oss.h"
#include "service.h"
#include "defs.h"
#include "lock.h"

#define SETOP(x) tp::servicemgr::get<tp::opmgr>()->set_op(x)
#define OPBLOCK(x) SETOP(L"");tp::opblock TP_UNIQUE_NAME(opblock_)(x)
#define CURRENT_OPLIST() tp::servicemgr::get<tp::opmgr>()->get_oplist(L" -> ")

#define SET_LONG_OP(x) tp::servicemgr::get<tp::opmgr>()->set_op(x, true)

namespace tp
{
	class opmgr : public service_impl<SID_OPMGR>
	{
		typedef std::list<std::wstring> strlist_t;
		struct oplist
		{
			strlist_t lst;
			std::wstring op;
		};
		typedef std::map<threadid_t, oplist*> opmap_t;
	public:
		~opmgr()
		{
			free();
		}

		void push_block(const std::wstring& op)
		{
			get_current_oplist()->lst.push_back(op);
		}
		void pop_block()
		{
			get_current_oplist()->lst.pop_back();
		}
		void set_op(const std::wstring& op, bool display = false)
		{
			get_current_oplist()->op = op;
			if (display)
			{
				wprintf(L"%s...\n", op.c_str());
			}
		}
		std::wstring get_oplist(const std::wstring& sep) const
		{
			std::wstring lstr;

			const oplist* lst = get_current_oplist();
			if (lst)
			{
				for (strlist_t::const_iterator it = lst->lst.begin(); it != lst->lst.end(); ++it)
				{
					if (!lstr.empty()) lstr += sep;
					lstr += *it;
				}
				if (!lst->op.empty())
				{
					if (!lstr.empty()) lstr += sep;
					lstr += lst->op;
				}
			}

			return lstr;
		}

	private:
		opmap_t m_obmap;
		mutable critical_section_lock m_lock;

		oplist* get_current_oplist()
		{
			autolocker<critical_section_lock> locker(m_lock);

			oplist*& lst = m_obmap[os::current_tid()];
			if (!lst)
			{
				lst = new oplist;
			}

			return lst;
		}
		oplist* get_current_oplist() const
		{
			autolocker<critical_section_lock> locker(m_lock);

			opmap_t::const_iterator it = m_obmap.find(os::current_tid());
			if (it != m_obmap.end())
			{
				return it->second;
			}

			return 0;
		}

		void free()
		{
			for (opmap_t::const_iterator it = m_obmap.begin(); it != m_obmap.end(); ++it)
			{
				delete it->second;
			}
			m_obmap.clear();
		}
	};

	DEFINE_SERVICE(opmgr, L"Operation Manager");

	class opblock
	{
	public:
		opblock(const std::wstring& op)
		{
			servicemgr::get<opmgr>()->push_block(op);
		}
		~opblock()
		{
			servicemgr::get<opmgr>()->pop_block();
		}
	};
}

#endif