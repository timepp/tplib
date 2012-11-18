#pragma once

#include <service.h>
#include <unittest.h>


enum
{
	SID_Service1 = 1,
	SID_Service2
};

class reader : public tp::service_impl<SID_Service1>
{
public:
	void setval(int val) { m_val = val; }
	int getval() const { return m_val; }
private:
	int m_val;
};
class writer : public tp::service_impl<SID_Service2>
{
};

DEFINE_SERVICE(reader, L"Reader");
DEFINE_SERVICE(writer, L"Writer");

/// service 必须是最后一个testcase
TPUT_DEFINE_BLOCK(L"service", L"")
{
	bool unexpected_exception = false;
	try
	{

		{
			reader* r = tp::servicemgr::get<reader>();
			r->setval(100);
		}
		{
			reader* r = tp::servicemgr::get<reader>();
			TPUT_EXPECT(r->getval() == 100, L"经servicemgr的get接口返回的是同一service实例");
		}

		TPUT_EXPECT_EXCEPTION(tp::servicemgr::instance().add_service_info(1000, NULL, NULL, NULL), tp::servicemgr::service_id_invalid, L"service_id 超出范围时抛异常");

		tp::servicemgr::instance().destroy_all_services();
	
	} catch (tp::exception&)
	{
		unexpected_exception = true;
	}

	TPUT_EXPECT(!unexpected_exception, L"不应该有未期待的exception");
}
