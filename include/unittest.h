#include <stdio.h>
#include <list>
#include <string>

namespace tp
{
	struct TestResult
	{
		int testid;
		const wchar_t* operation;
		bool success;
	};
	struct TestBlock
	{
		int blockid;
		const wchar_t* name;
		const wchar_t* tags;
		void (*func)();
	};

	struct TestOutput
	{
		virtual void BlockBegin(const TestBlock& block) = 0;
		virtual void OutputResult(const TestResult& res) = 0;
		virtual void TestEnd(int total, int succeeded) = 0;
	};

	class ConsoleTestOutput : public TestOutput
	{
	public:
		virtual void OutputResult(const TestResult& res)
		{
			wprintf_s(L"[%03d] %-4s | %s\n", res.testid, res.success? L"OK":L"FAIL", res.operation);
		}
		virtual void TestEnd(int total, int succeeded)
		{
			std::wstring spliteline(79, L'=');
			wprintf_s(L"\n\n%s\n", spliteline.c_str());
			wprintf_s(L"测试结束，%d 成功，%d 失败\n", succeeded, total - succeeded);
			wprintf_s(L"%s\n", spliteline.c_str());
		}
		virtual void BlockBegin(const TestBlock& block)
		{
			std::wstring spliteline(79, L'=');
			wprintf_s(L"\n[SUITE %03d] %s\n%s\n", block.blockid, block.name, spliteline.c_str());
		}
	private:
		
	};

	class unittest
	{
	public:
		void check_HR(HRESULT hr, const wchar_t* op)
		{
			output(m_testid++, SUCCEEDED(hr), op);
		}
		void check(bool condition, const wchar_t* op)
		{
			output(m_testid++, condition, op);
		}
		void ResetCounter()
		{
			m_testid = 1;
		}

		void add_test_block(const wchar_t* name, const wchar_t* tags, void (*func)())
		{
			if (!name) return;

			// 以#开头的name相当于没有启用，省去要注释一大段代码的麻烦
			if (name[0] == L'#') return;

			TestBlock block;
			block.blockid = m_blockid++;
			block.name = name;
			block.tags = tags;
			block.func = func;
			m_blocks.push_back(block);
		}

		void run_test(int blockid, const wchar_t* name, const wchar_t* tags)
		{
			for (TestBlockList::const_iterator it = m_blocks.begin(); it != m_blocks.end(); ++it)
			{
				if (blockid != 0 && it->blockid != blockid) continue;
				if (name != NULL && it->name != name) continue;

				m_output->BlockBegin(*it);
				(*it->func)();
			}
		}
		void run_test()                             { return run_test(0, NULL, NULL); }
		void run_test(int blockid)                  { return run_test(blockid, NULL, NULL); }
		void run_test(const wchar_t* name)          { return run_test(0, name, NULL); }
//		void run_test(const wchar_t* tags)          { return run_test(0, NULL, tags); }

		static unittest& instance()
		{
			static unittest ut;
			return ut;
		}

	private:
		unittest(): m_testid(1), m_total_count(0), m_success_count(0), m_blockid(1)
		{
			m_output = new ConsoleTestOutput;
		}
		~unittest()
		{
			m_output->TestEnd(m_total_count, m_success_count);
			delete m_output;
		}

		void output(int testid, bool success, const wchar_t* op)
		{
			m_total_count++;
			if (success)
				m_success_count++;

			TestResult res;
			res.testid = testid;
			res.success = success;
			res.operation = op;
			output(res);
		}
		void output(const TestResult& res)
		{
			m_output->OutputResult(res);
		}

		TestOutput* m_output;
		int m_testid;
		int m_blockid;
		int m_total_count;
		int m_success_count;
		typedef std::list<TestBlock> TestBlockList;
		TestBlockList m_blocks;
	};

	class test_register
	{
	public:
		test_register(void (*func)(), const wchar_t* name, const wchar_t* tags)
		{
			unittest::instance().add_test_block(name, tags, func);
		}
	};
}

#ifndef TPUT_WIDESTRING
#define TPUT_WIDESTRING2(str) L##str
#define TPUT_WIDESTRING(str) TPUT_WIDESTRING2(str)
#endif

#ifndef TPUT_STRINGIZE
#define TPUT_STRINGIZE2(x) #x
#define TPUT_STRINGIZE(x) TPUT_STRINGIZE2(x)
#endif

#define TPUT_CONCAT_INNER(a,b) a##b
#define TPUT_CONCAT(a,b) TPUT_CONCAT_INNER(a,b)
#define TPUT_NAME(prefix) TPUT_CONCAT(TPUT_CONCAT(prefix, _),__LINE__)


#define TPUT_EXPECT(statement, msg)          \
	do {                                \
		bool ret___ = (statement);         \
		const wchar_t* op___ = msg? msg : TPUT_WIDESTRING(#statement); \
		tp::unittest::instance().check(ret___, op___);   \
	} while (0);

#define TPUT_EXPECT_EXCEPTION(statement, exception_type, msg) \
	do { \
		std::wstring op___ = msg? msg: std::wstring(TPUT_WIDESTRING(#statement)) + L" --throw-> " + TPUT_WIDESTRING(#exception_type); \
		bool exception_thrown___ = false; \
		try { statement; } catch (exception_type&) { \
			exception_thrown___ = true; \
		} \
		tp::unittest::instance().check(exception_thrown___, op___.c_str()); \
	} while (0);

#define TPUT_REGISTER_BLOCK(f, name, tags) \
	tp::test_register TPUT_NAME(TPUT_reg_var_)(&f, name, tags)

#define TPUT_DEFINE_BLOCK(name, tags) \
	void TPUT_NAME(TPUT_MODNAME)(); \
	TPUT_REGISTER_BLOCK(TPUT_NAME(TPUT_MODNAME), name, tags); \
	void TPUT_NAME(TPUT_MODNAME)()
