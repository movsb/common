#pragma once

#ifdef _DEBUG
#define debug_out(x) printf##x
#else
#define debug_out(x) 
#endif


//////////////////////////////////////////////////////////////////////////
// Smart Assert

// #ifdef _DEBUG
// #define __SMART_DEBUG
// #else
// #undef __SMART_DEBUG
// #endif

#define __SMART_DEBUG

#ifdef __SMART_DEBUG

extern std::ofstream __debug_file;

class CSmartAssert
{
public:
	CSmartAssert()
		: __SMART_ASSERT_A(*this)
		, __SMART_ASSERT_B(*this)
	{}
	CSmartAssert& __SMART_ASSERT_A;
	CSmartAssert& __SMART_ASSERT_B;
	CSmartAssert& Context(const char* expr, const char* file, int line)
	{
		std::stringstream ss;
		ss << expr << "\nfile: " << file << "\nline: " << line;
		m_expr = ss.str();
		__debug_file << "\r\n[" << file << ", " << line << "]\r\n";
		__debug_file << "Assertion failed: " << expr << "\r\n";
		__debug_file.flush();
		return *this;
	}

	// to use this , you must have an operator<<(ostream&, your_class) overloaded
	template <class T>
	CSmartAssert& Out(const char* str, T t)
	{
		std::stringstream ss;
		ss << "\n\t" << str << " = " << t << "\n";
		m_expr += ss.str();
		__debug_file << "\t" << str << " = " << t << "\r\n";
		__debug_file.flush();
		return *this;
	}
	void Warning()
	{
		std::string str("警告: 应用程序遇到警告性错误!\n\n");
		str += m_expr;
		str += "\n\n请查看debug.txt调试输出!";
		::MessageBox(nullptr, str.c_str(), nullptr, MB_ICONWARNING);
	}
	void Fatal()
	{
		std::string str("错误: 应用程序遇到严重性错误!\n\n");
		str += m_expr;
		str += "\n\n请查看debug.txt调试输出!";
		::MessageBox(nullptr, str.c_str(), nullptr, MB_ICONERROR);
	}
	void Stop()
	{
		std::string str("停止: 应用程序错误!\n\n请立即取消当前操作! 否则可能造成不可想像的后果!\n\n");
		str += m_expr;
		str += "\n\n请查看debug.txt调试输出!";
		::MessageBox(nullptr, str.c_str(), nullptr, MB_ICONEXCLAMATION);
	}

private:
	std::string m_expr;
};

#define __SMART_ASSERT_A(x) __SMART_ASSERT(x,B)
#define __SMART_ASSERT_B(x) __SMART_ASSERT(x,A)

#define __SMART_ASSERT(x,next) \
	__SMART_ASSERT_A.Out(#x, x).__SMART_ASSERT_##next

#define SMART_ASSERT(expr) \
	if(expr) ;\
	else CSmartAssert().Context(#expr, __FILE__, __LINE__).__SMART_ASSERT_A

#define SMART_ENSURE(a,b) SMART_ASSERT((a)b) // or (a) ## b

#else // !SMART_DEBUG

#define SMART_ASSERT __SMART_ASSERT/
#define __SMART_ASSERT /

#define SMART_ENSURE(a,b) a;SMART_ASSERT

#endif // SMART_DEBUG
