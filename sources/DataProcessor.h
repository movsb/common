#pragma once

namespace Common{
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// 数据处理器接口: 比如文本管理器 16进制管理器, 由下面的数据接收器调用
	// 一般由需要有后续处理的数据处理继承此接口, 否则可以直接处理, 比如'\t'的处理就不需要
	class i_data_processor
	{
	public:
		// 处理部分数据: 
		//		follow:	前一次调用是否为本处理函数, 也即后续连续调用
		//		ba:		Byte Array, 字节数组
		//		cb:		Count of Bytes, 字节数
		//		*pn:	本次处理了多少数据
		// 返回值:
		//		bool:	是否希望继续处理, 影响下一次调用时follow的值
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn) = 0;

		// 重置数据处理缓冲: 比如, 在关闭串口后, 或清空16进制数据后
		virtual void reset_buffer() = 0;

		virtual operator i_data_processor*() = 0;
	};

	// 数据接收器接口: 串口在接收到数据后调用所有的接收器
	class i_data_receiver
	{
	public:
		// 数据接收函数, 读线程接收到数据时调用此函数
		// ba指向的内容不应该被更改!
		virtual void receive(const unsigned char* ba, int cb) = 0;
		virtual void reset_buffer() = 0;
	protected:
		// 一个调用处理器并设置剩余数据与后续调用标志的辅助函数
		virtual bool process(i_data_processor* proc, bool follow, const unsigned char** pba, int* pcb, i_data_processor** ppre)
		{
			int n;
			bool c = proc->process_some(follow, *pba, *pcb, &n);
			SMART_ASSERT(n <= *pcb)(n)(*pcb).Fatal();
			*pba += n;
			*pcb -= n;
			*ppre = c ? proc : NULL;
			return c;
		}
	};

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	class c_single_byte_processor : public i_data_processor
	{
	public:
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer() {}

	public:
		Window::c_rich_edit*	_richedit;
	};

	class c_crlf_data_processor : public i_data_processor
	{
	public:
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();

		c_crlf_data_processor()
			: _post_len(0)
		{}

	protected:
		int _post_len;
		c_byte_array<16, 64> _data;
	public:
		Window::c_rich_edit* _richedit;
	};

	// Linux控制字符处理
	// http://www.cnblogs.com/memset/p/linux_printf_with_color.html
	// http://ascii-table.com/ansi-escape-sequences.php
	class c_escape_data_processor : public i_data_processor
	{
	public:
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();

	protected:
		enum lcs_state{
			LCS_NONE,LCS_ESC,LCS_BRACKET,LCS_VAL,LCS_SEMI,
			LCS_H,LCS_f,
			LCS_A,LCS_B,LCS_C,LCS_D,
			LCS_s,LCS_u,LCS_j,LCS_K,
			LCS_h,LCS_l,LCS_EQU,
			LCS_m,LCS_P
		} _state;
		c_byte_array<64, 64> _data;		// 数据栈
		std::vector<lcs_state> _stack;	// 状态栈
	public:
		Window::c_rich_edit* _richedit;
	};

	class c_ascii_data_processor : public i_data_processor
	{
	public:
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();

	public:
		Window::c_rich_edit* _richedit;
	};

	// 中文扩展ASCII字符处理 (CodePage936 compatible, EUC-CN)
	// http://en.wikipedia.org/wiki/GB_2312
	// http://zh.wikipedia.org/wiki/GB_2312
	// http://www.knowsky.com/resource/gb2312tbl.htm
	class c_gb2312_data_processor : public i_data_processor
	{
	public:
		c_gb2312_data_processor()
			: _lead_byte(0)
		{}
		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();

	public:
		unsigned char _lead_byte;			// 中文前导字符
		Window::c_rich_edit* _richedit;
	};

	//////////////////////////////////////////////////////////////////////////
	class c_text_data_receiver : public i_data_receiver
	{
	public:
		c_text_data_receiver()
			: _pre_proc(0)
			, _rich_editor(0)
		{}

		// interface i_data_receiver
		virtual void receive(const unsigned char* ba, int cb);
		virtual void reset_buffer(){
			_pre_proc = 0;
			_proc_ascii.reset_buffer();
			_proc_escape.reset_buffer();
			_proc_crlf.reset_buffer();
			_proc_gb2312.reset_buffer();
		}
		void set_editor(Window::c_rich_edit* edt) {
			_rich_editor = edt;
			_proc_byte._richedit = edt;
			_proc_ascii._richedit = edt;
			_proc_escape._richedit = edt;
			_proc_crlf._richedit = edt;
			_proc_gb2312._richedit = edt;
		}

	protected:
		Window::c_rich_edit*		_rich_editor;
		i_data_processor*			_pre_proc;
		c_single_byte_processor		_proc_byte;
		c_crlf_data_processor		_proc_crlf;
		c_escape_data_processor		_proc_escape;
		c_ascii_data_processor		_proc_ascii;
		c_gb2312_data_processor		_proc_gb2312;
	};

	//////////////////////////////////////////////////////////////////////////
	class c_hex_data_processor : public i_data_processor
	{
	public:
		c_hex_data_processor()
			: _editor(0)
			, _count(0)
		{}

		virtual operator i_data_processor*() { return static_cast<i_data_processor*>(this); }
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn);
		virtual void reset_buffer();
		void set_count(int n) { _count = n; }

		Window::c_edit*			_editor;

	private:
		int _count;
	};

	class c_hex_data_receiver : public i_data_receiver
	{
	public:
		c_hex_data_receiver()
			: _pre_proc(0)
			, _editor(0)
		{}

		virtual void receive(const unsigned char* ba, int cb);
		virtual void reset_buffer(){
			_pre_proc = 0;
			_proc_hex.reset_buffer();
		}
		void set_editor(Window::c_edit* edt) {
			_editor = edt;
			_proc_hex._editor = edt;
		}

		void set_count(int n){ _proc_hex.set_count(n); }
	protected:
		Window::c_edit*			_editor;
		i_data_processor*		_pre_proc;
		c_hex_data_processor	_proc_hex;
	};

	//////////////////////////////////////////////////////////////////////////
	class c_file_data_receiver : public i_data_receiver {
	public:
		c_file_data_receiver()
		{}

		virtual void receive(const unsigned char* ba, int cb) {
			_data.append(ba, cb);
		}
		virtual void reset_buffer() {
			_data.empty();
		}

		size_t size() { 
			return _data.get_size(); 
		}
		unsigned char* data() {
			return reinterpret_cast<unsigned char*>(_data.get_data());
		}
	protected:
		c_byte_array<1 << 20, 1 << 20> _data;
	};
}
