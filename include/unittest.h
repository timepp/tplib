#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <list>
#include <string>

namespace tp
{
	struct TestBlock
	{
		int blockid;
		const wchar_t* name;
		const wchar_t* tags;
		void (*func)();
	};
	struct TestResult
	{
		int testid;
		std::wstring operation;
		std::wstring comment;
		bool success;
		const TestBlock* block;
	};

	struct TestOutput
	{
		virtual void BlockBegin(const TestBlock& block) = 0;
		virtual void OutputResult(const TestResult& res) = 0;
		virtual void TestEnd(int total, int succeeded) = 0;
        virtual ~TestOutput() {}
	};

	class ConsoleTestOutput : public TestOutput
	{
	public:
		virtual void OutputResult(const TestResult& res)
		{
			wprintf_s(L"[%03d] %-4s | %s\n", res.testid, res.success? L"OK":L"FAIL", res.operation.c_str());
		}
		virtual void TestEnd(int total, int succeeded)
		{
			std::wstring spliteline(79, L'=');
			wprintf_s(L"\n%s\n%s\n", spliteline.c_str(), spliteline.c_str());
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
		void check(bool condition, const wchar_t* op, const wchar_t* comment = L"")
		{
			output(m_testid++, condition, op, comment);
		}
		void ResetCounter()
		{
			m_testid = 1;
		}

		void set_test_output(TestOutput* o)
		{
			m_output = o;
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

		void run_test(int blockid, const wchar_t* name, const wchar_t* /*tags*/)
		{
			m_total_count = 0;
			m_success_count = 0;
			for (TestBlockList::const_iterator it = m_blocks.begin(); it != m_blocks.end(); ++it)
			{
				if (blockid != 0 && it->blockid != blockid) continue;
				if (name != NULL && !wildcard_match(it->name, name)) continue;

				m_current_block = &(*it);
				m_output->BlockBegin(*it);
				(*it->func)();
			}
			m_output->TestEnd(m_total_count, m_success_count);
		}
		void run_test()                             { return run_test(0, NULL, NULL); }
		void run_test(int blockid)                  { return run_test(blockid, NULL, NULL); }
		void run_test(const wchar_t* name)          { return run_test(0, name, NULL); }
//		void run_test(const wchar_t* tags)          { return run_test(0, NULL, tags); }

		static unittest& instance()
		{
			__pragma(warning(push)) __pragma(warning(disable:4640))
			/// 这里有一个警告4640，说构造静态局部变量是非线程安全的
			/// 在unittest的场景下，ut的构造是被一系列用于注册的全局变量的构造触发的，构造ut时一定不存在其他的线程
			/// 所以在这里，这个警告可以被禁用
			static unittest ut;
			__pragma(warning(pop))

			return ut;
		}

		static void expect(bool cond, const wchar_t* msg)
		{
			tp::unittest::instance().check(cond, msg, L"");
		}
				static void expect(bool cond, const wchar_t* msg, const wchar_t* comment_fmt, ...)
		{
			va_list args;
			va_start(args, comment_fmt);
			wchar_t comment[1024];
			_vsnwprintf_s(comment, _TRUNCATE, comment_fmt, args);
			va_end(args);
			tp::unittest::instance().check(cond, msg, comment);
		}

	private:
		unittest(): m_testid(1), m_total_count(0), m_success_count(0), m_blockid(1), m_output(NULL)
		{
			
		}
		~unittest()
		{
		}

		void output(int testid, bool success, const wchar_t* op, const wchar_t* comment)
		{
			m_total_count++;
			if (success)
				m_success_count++;

			TestResult res;
			res.testid = testid;
			res.success = success;
			res.operation = op;
			res.comment = comment;
			res.block = m_current_block;
			output(res);
		}
		void output(const TestResult& res)
		{
			m_output->OutputResult(res);
		}
		bool wildcard_match(const wchar_t* str, const wchar_t* pattern)
		{
			if (!*pattern) return true;
			while (*str)
			{
				if(*pattern == '*')
				{
					do{pattern++;}while(*pattern=='*');
					while(*str)
						if(wildcard_match(str++,pattern))
							return true; 
					return false;
				}
				else if(*str == *pattern || *pattern =='?')
				{
					pattern++;
					str++;
				}
				else return false;
			}
			while(*pattern == '*') ++pattern;
			return !*pattern;
		}

		TestOutput* m_output;
		int m_testid;
		int m_blockid;
		int m_total_count;
		int m_success_count;
		typedef std::list<TestBlock> TestBlockList;
		TestBlockList m_blocks;
		const TestBlock* m_current_block;
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


#define TPUT_EXPECT(statement, msg)          \
	{                                \
		bool ret___ = (statement);         \
		const wchar_t* op___ = msg? msg : TPUT_WIDESTRING(#statement); \
		tp::unittest::instance().check(ret___, op___);   \
	} 

#define TPUT_EXPECT_EXCEPTION(statement, exception_type, msg) \
	{ \
		std::wstring op___ = *msg? msg L":" TPUT_WIDESTRING(#exception_type): std::wstring(TPUT_WIDESTRING(#statement)) + L" --throw-> " + TPUT_WIDESTRING(#exception_type); \
		bool exception_thrown___ = false; \
		try { statement; } catch (exception_type&) { \
			exception_thrown___ = true; \
		} \
		tp::unittest::instance().check(exception_thrown___, op___.c_str()); \
	}

#define TPUT_EXPECT_NO_EXCEPTION(statement, msg) \
	{ \
		const wchar_t* op___ = msg? msg : TPUT_WIDESTRING(#statement); \
		bool exception_thrown___ = false; \
		try { statement; } catch (...) { \
			exception_thrown___ = true; \
		} \
		tp::unittest::instance().check(!exception_thrown___, op___); \
	}

#define TPUT_DEFINE_BLOCK3(name, tags, func, var) \
	static void func(); \
	static tp::test_register var(&func, name, tags); \
	void func()

#define TPUT_DEFINE_BLOCK(name, tags) TPUT_DEFINE_BLOCK2(name, tags, tput_test_func_, tput_register_, __COUNTER__)
#define TPUT_DEFINE_BLOCK2(name, tags, prefixf, prefixv, c) TPUT_DEFINE_BLOCK3(name, tags, TPUT_CONCAT(prefixf, c), TPUT_CONCAT(prefixv, c))

