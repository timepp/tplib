#pragma once

#include <service.h>
#include <unittest.h>
#include <sstream>

enum
{
	SID_Begin = 100,
	SID_MyService,
	SID_Service1,
	SID_Service2,
	SID_SelfDependent,
};

class service_lifetracer
{
public:
	service_lifetracer(std::wstring* o, tp::sid_t sid) : m_output(o), m_sid(sid)
	{
		if (m_output)
		{
			std::wostringstream ss;
			ss << m_sid << L" ";
			*m_output += ss.str();
		}
	}
	~service_lifetracer()
	{
		if (m_output)
		{
			std::wostringstream ss;
			ss << L"~" << m_sid << L" ";
			*m_output += ss.str();
		}
	}
private:
	std::wstring* m_output;
	tp::sid_t m_sid;
};

template <
	tp::sid_t sid,
	tp::sid_t cd1 = 0, tp::sid_t cd2 = 0, tp::sid_t cd3 = 0,
	tp::sid_t dd1 = 0, tp::sid_t dd2 = 0, tp::sid_t dd3 = 0,
	tp::sid_t cu1 = 0, tp::sid_t cu2 = 0, tp::sid_t cu3 = 0,
	tp::sid_t du1 = 0, tp::sid_t du2 = 0, tp::sid_t du3 = 0
> class testservice: public tp::service_impl<sid>
{
public:
	typedef testservice<sid, cd1, cd2, cd3, dd1, dd2, dd3, cu1, cu2, cu3, du1, du2, du3> service_type;

	static size_t get_create_dependencies(tp::sid_t* sids, size_t len)
	{
		return tp::helper::copy_dependencies(sids, len, cd1, cd2, cd3, 0);
	}
	static size_t get_destroy_dependencies(tp::sid_t* sids, size_t len)
	{
		return tp::helper::copy_dependencies(sids, len, dd1, dd2, dd3, 0);
	}

	testservice(tp::servicemgr* mgr = NULL, std::wstring* tracestr = NULL) : m_mgr(mgr), m_tracer(tracestr, sid)
	{
		tp::sid_t ss[] = {cd1, cd2, cd3, cu1, cu2, cu3};
		use_services(ss, _countof(ss));
	}

	~testservice()
	{
		tp::sid_t ss[] = {dd1, dd2, dd3, du1, du2, du3};
		use_services(ss, _countof(ss));
	}

	void setdata(int data) { m_data = data; }
	int getdata() const { return m_data; }

private:
	tp::servicemgr* m_mgr;
	int m_data;
	service_lifetracer m_tracer;

	void use_services(tp::sid_t* sids, size_t len)
	{
		for (size_t i = 0; i < len; i++)
		{
			tp::sid_t s = sids[i];
			if (s > 0) m_mgr->get_service(s);
		}
	}

public:
	class factory : public tp::service_factory_impl<service_type>
	{
		tp::servicemgr* m_mgr;
		std::wstring* m_tracestr;
	public:
		factory(tp::servicemgr* mgr, std::wstring* tracestr) 
			: m_mgr(mgr) 
			, m_tracestr(tracestr)
			, tp::service_factory_impl<service_type>(L"deptest", L"")
		{}
		virtual tp::service* create()
		{
			tp::service* s = new service_type(m_mgr, m_tracestr);
			s->addref();
			return s;
		}
	};
};

class GlobalTestService : public tp::service_impl<SID_Service1>
{
	TP_SERVICE_DEFINE_CREATE_DEPENDENCIES();
	TP_SERVICE_DEFINE_DESTROY_DEPENDENCIES();
public:
	int getdata() const { return 123; }
};

TP_DEFINE_GLOBAL_SERVICE(GlobalTestService, L"xxx");

TPUT_DEFINE_BLOCK(L"service", L"")
{
	tp::servicemgr mgr;
	std::wstring tracestr, tmp;
	
	typedef testservice<1> datamgr;
	mgr.register_service(new datamgr::factory(&mgr, &tracestr));

	service_ptr<datamgr> r = mgr.get_service<datamgr>();
	r->setdata(100);
	r = NULL;
	r = mgr.get_service<datamgr>();
	TPUT_EXPECT(r->getdata() == 100, L"��service���ص���ͬһʵ��");
	mgr.destroy_all_services();
	tmp = tracestr;
	r = NULL; 
	TPUT_EXPECT(tmp == L"1 " && tracestr == L"1 ~1 ", L"service�����ü�����������");
	
	mgr.clear();
	mgr.register_service(new testservice<1, 2,3,6,  0,0,0>::factory(&mgr, &tracestr));
	mgr.register_service(new testservice<2, 4,0,0,  4,6,0>::factory(&mgr, &tracestr));
	mgr.register_service(new testservice<3, 6,0,0,  4,5,2>::factory(&mgr, &tracestr));
	mgr.register_service(new testservice<4, 5,6,0,  1,5,0>::factory(&mgr, &tracestr));
	mgr.register_service(new testservice<5, 0,0,0,  6,0,0>::factory(&mgr, &tracestr));
	mgr.register_service(new testservice<6, 5,0,0,  1,0,0>::factory(&mgr, &tracestr));

	tracestr.clear();
	mgr.get_service(1);
	TPUT_EXPECT(tracestr == L"5 6 4 2 3 1 ", L"�����ķ����ܹ���˳����ȷ����");
	tracestr.clear();
	mgr.destroy_all_services();
	TPUT_EXPECT(tracestr == L"~3 ~2 ~4 ~5 ~6 ~1 ", L"�����ܹ���������ϵ��ȷ����");

	tracestr.clear();
	mgr.clear();
	mgr.register_service(new testservice<2, 0,0,0, 1,0,0, 0,0,0, 1,0,0>::factory(&mgr, &tracestr));
	mgr.register_service(new testservice<1>::factory(&mgr, &tracestr));
	mgr.get_service(2);
	tmp = tracestr;
	mgr.destroy_all_services();
	TPUT_EXPECT(tmp == L"2 " && tracestr == L"2 1 ~2 ~1 ", L"service������ʱ�������ķ���δ����������ȷ����������");

	mgr.clear();
}

TPUT_DEFINE_BLOCK(L"service.exception", L"")
{
	tp::servicemgr mgr;

	TPUT_EXPECT_EXCEPTION(mgr.get_service(1), tp::service_id_invalid, L"ȡ�����ڵķ�����ȷ���쳣");
	TPUT_EXPECT_EXCEPTION(mgr.get_service(100000), tp::service_id_invalid, L"sid����,���쳣");
	TPUT_EXPECT_EXCEPTION(mgr.get_service(-1), tp::service_id_invalid, L"sidС��0,���쳣");

	mgr.register_service(new testservice<1>::factory(&mgr, NULL));
	TPUT_EXPECT_EXCEPTION(
		mgr.register_service(new testservice<1>::factory(&mgr, NULL)),
		tp::service_id_conflict,
		L"�ظ�ע��ͬһ����ID�����쳣");

	mgr.clear();
	mgr.register_service(new testservice<1, 0,0,0, 0,0,0, 2,0,0>::factory(&mgr, NULL));
	mgr.register_service(new testservice<2, 0,0,0, 0,0,0, 0,0,0, 3,0,0>::factory(&mgr, NULL));
	TPUT_EXPECT_EXCEPTION(
		mgr.get_service(1),
		tp::service_out_of_dependency, 
		L"����ʱ������δ����������������ȷ���쳣");
	mgr.get_service(2);
	TPUT_EXPECT_EXCEPTION(
		mgr.destroy_all_services(),
		tp::service_out_of_dependency,
		L"����ʱ������δ����������������ȷ���쳣");

	mgr.clear();
	mgr.register_service(new testservice<1, 1,0,0, 0,0,0, 1,0,0>::factory(&mgr, NULL));

	TPUT_EXPECT_EXCEPTION(
		mgr.get_service(1),
		tp::service_cycle_create, 
		L"����ʱ���õ�����(a->a)����ȷ���쳣");

	mgr.clear();
	mgr.register_service(new testservice<1, 2,0,0, 0,0,0, 2,0,0>::factory(&mgr, NULL));
	mgr.register_service(new testservice<2, 1,0,0, 0,0,0, 1,0,0>::factory(&mgr, NULL));
	TPUT_EXPECT_EXCEPTION(
		mgr.get_service(1),
		tp::service_cycle_create,
		L"����ʱ��������(a->b,b->a)����ȷ���쳣");

	mgr.clear();
	mgr.register_service(new testservice<1, 2,0,0, 0,0,0, 2,0,0>::factory(&mgr, NULL));
	mgr.register_service(new testservice<2, 3,0,0, 0,0,0, 3,0,0>::factory(&mgr, NULL));
	mgr.register_service(new testservice<3, 1,0,0, 0,0,0, 1,0,0>::factory(&mgr, NULL));
	TPUT_EXPECT_EXCEPTION(
		mgr.get_service(1),
		tp::service_cycle_create,
		L"����ʱѭ������(a->b->c->a)����ȷ���쳣");

	mgr.clear();
	mgr.register_service(new testservice<1>::factory(&mgr, NULL));
	mgr.get_service(1);
	mgr.destroy_all_services();
	TPUT_EXPECT_EXCEPTION(
		mgr.get_service(1),
		tp::service_destroyed,
		L"�������ٺ��Ի�ȡ�����쳣");

	mgr.clear();
	mgr.register_service(new testservice<1, 0,0,0, 1,0,0, 0,0,0, 1,0,0>::factory(&mgr, NULL));
	mgr.get_service(1);
	TPUT_EXPECT_EXCEPTION(
		mgr.destroy_all_services(),
		tp::service_cycle_destroy, 
		L"����ʱ���õ�����(a->a)����ȷ���쳣");

	mgr.clear();
	mgr.register_service(new testservice<1, 0,0,0, 2,0,0, 0,0,0, 2,0,0>::factory(&mgr, NULL));
	mgr.register_service(new testservice<2, 0,0,0, 1,0,0, 0,0,0, 1,0,0>::factory(&mgr, NULL));
	mgr.get_service(1);
	mgr.get_service(2);
	TPUT_EXPECT_EXCEPTION(
		mgr.destroy_all_services(),
		tp::service_cycle_destroy,
		L"����ʱ��������(a->b,b->a)����ȷ���쳣");

	mgr.clear();
	mgr.register_service(new testservice<1, 0,0,0, 2,0,0, 0,0,0, 2,0,0>::factory(&mgr, NULL));
	mgr.register_service(new testservice<2, 0,0,0, 3,0,0, 0,0,0, 3,0,0>::factory(&mgr, NULL));
	mgr.register_service(new testservice<3, 0,0,0, 1,0,0, 0,0,0, 1,0,0>::factory(&mgr, NULL));
	mgr.get_service(1);
	mgr.get_service(2);
	mgr.get_service(3);
	TPUT_EXPECT_EXCEPTION(
		mgr.destroy_all_services(),
		tp::service_cycle_destroy,
		L"����ʱѭ������(a->b->c->a)����ȷ���쳣");
}

TPUT_DEFINE_BLOCK(L"service.global", L"")
{
	typedef testservice<SID_MyService> MyService;
	tp::global_servicemgr().register_service(new MyService::factory(NULL, NULL));
	service_ptr<MyService> s = tp::global_service<MyService>();
	s->setdata(100);
	s = tp::global_service<MyService>();
	TPUT_EXPECT(s->getdata() == 100, L"��ȫ�ַ��������ȡ������ͬSID����ͬ�ķ������");
	TPUT_EXPECT(s->addref() == 3 && s->release() == 2, L"ȫ�ַ�������ü�������");

	TPUT_EXPECT(tp::global_service<GlobalTestService>()->getdata() == 123, L"ȫ��ע�������������");
}

