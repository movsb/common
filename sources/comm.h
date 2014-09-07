#pragma once

namespace Common {
	// 发送, 接收, 未发送数据计数器接口
	class i_data_counter
	{
	public:
		// 读, 写, 未写
		virtual void update_counter(long rd, long wr, long uw) = 0;
	};

	// 数据计数器类
	class c_data_counter
	{
	public:
		c_data_counter()
			: _updater(0)
		{
			reset_all();
		}

		~c_data_counter()
		{
			reset_all();
		}

		void set_updater(i_data_counter* udt) { _updater = udt; }
		void call_updater(){
			SMART_ASSERT(_updater != NULL).Warning();
			_lock.lock();
			_updater->update_counter(_n_read, _n_write, _n_unwr);
			_lock.unlock();
		}

		void reset_all(){
			InterlockedExchange(&_n_read, 0);
			InterlockedExchange(&_n_unwr, 0);
			InterlockedExchange(&_n_write, 0);
		}

		void reset_wr_rd(){
			InterlockedExchange(&_n_read, 0);
			InterlockedExchange(&_n_write, 0);
		}

		void reset_unsend()			{ InterlockedExchange(&_n_unwr, 0); }
		void add_send(int n)		{ InterlockedExchangeAdd(&_n_write, n); }
		void add_recv(int n)		{ InterlockedExchangeAdd(&_n_read, n); }
		void add_unsend(int n)		{ InterlockedExchangeAdd(&_n_unwr, n); }
		void sub_unsend(int n)		{ InterlockedExchangeAdd(&_n_unwr, -n); }

	protected:
		c_critical_locker	_lock;
		i_data_counter*		_updater;
		volatile long		_n_write;
		volatile long		_n_read;
		volatile long		_n_unwr;
	};

	// 数据处理器接口: 比如文本管理器 16进制管理器, 由下面的数据接收器调用
	class i_data_processor
	{
	public:
		virtual bool process_some(bool follow, const unsigned char* ba, int cb, int* pn) = 0;
		virtual operator i_data_processor*() = 0;
	};

	// 数据接收器接口: 串口在接收到数据后调用所有的接收器
	class i_data_receiver
	{
	public:
		virtual void receive(const unsigned char* ba, int cb) = 0;
	protected:
		virtual bool process(i_data_processor* proc, bool follow, const unsigned char** pba, int* pcb, i_data_processor** ppre)
		{
			int n;
			bool c = proc->process_some(follow, *pba, *pcb, &n);
			assert(n <= *pcb);
			*pba += n;
			*pcb -= n;
			*ppre = c ? proc : NULL;
			return c;
		}
	};


	//////////////////////////////////////////////////////////////////////////
	// 以下定义 发送数据封装相关类和结构

	// 默认发送缓冲区大小, 超过此大小会自动从内存分配
	const int csdp_def_size = 1024;
	enum csdp_type{
		csdp_local,		// 本地包, 不需要释放
		csdp_alloc,		// 分配包, 在数据量大于默认缓冲区或内部缓冲区不够时被分配
		csdp_exit,		
	};
#pragma pack(push,1)
	// 基础发送数据包, 不包含缓冲区
	struct c_send_data_packet{
		csdp_type		type;			// 包类型
		list_s			_list_entry;	// 用于添加到发送队列
		bool			used;			// 是否已被使用
		int				cb;				// 数据包数据长度
		unsigned char	data[0];
	};

	// 扩展发送数据包, 有一个 csdp_def_size 大小的缓冲区
	struct c_send_data_packet_extended{
		csdp_type		type;			// 包类型
		list_s			_list_entry;	// 用于添加到发送队列
		bool			used;			// 是否已被使用
		int				cb;				// 数据包数据长度
		unsigned char	data[csdp_def_size];
	};
#pragma pack(pop)

	// 发送数据包管理器, 用来将发送的数据包排成队列
	// 包管理器会被多个线程同时访问
	class c_data_packet_manager
	{
	public:
		c_data_packet_manager();
		~c_data_packet_manager();
		void					empty();
		c_send_data_packet*		alloc(int size);						// 通过此函数获取一个可以容纳指定大小数据的包
		void					release(c_send_data_packet* psdp);		// 返还一个包
		void					put(c_send_data_packet* psdp);			// 向发送队列尾插入一个新的数据包
		void					put_front(c_send_data_packet* psdp);	// 插入一个发送数据包到队列首, 优先处理
		c_send_data_packet*		get();									// 包获取者调用此接口取得数据包, 没有包时会被挂起
		c_send_data_packet*		query_head();
		HANDLE					get_event() const { return _hEvent; }

	private:
		c_send_data_packet_extended	_data[100];	// 预定义的本地包的个数
		c_critical_locker			_lock;		// 多线程锁
		HANDLE						_hEvent;	// 尝试get()可不能锁定, 因为其它地方要put()!
		list_s						_list;		// 发送数据包队列
	};

	//////////////////////////////////////////////////////////////////////////
	// 以下管理串口相关的一些对象, 如: 波特率列表 ...

	// 串口对象
	class t_com_item
	{
	public:
		t_com_item(int i,const char* s){_s = s; _i=i;}

		// 返回字符串部分: 比如: 无校验位
		std::string get_s() const {return _s;}
		// 返回整数部分 : 比如: NOPARITY(宏)
		int get_i() const {return _i;}

	protected:
		std::string _s;
		int _i;
	};

	// 刷新串口对象列表时需要用到的回调函数类型
	typedef void t_list_callback(void* ud, const t_com_item* t);

	// 串口对象刷新时的回调类型接口
	class i_com_list
	{
	public:
		virtual void callback(t_list_callback* cb, void* ud) = 0;
	};

	// 串口对象容器: 比如 保存系统所有的串口列表
	template<class T>
	class t_com_list : public i_com_list
	{
	public:
		void empty() {_list.clear();}
		void add(T t) {_list.push_back(t);}
		int size() {return _list.size();}
		const T& operator[](int i) {return _list[i];}

		// 更新对象列表, 比如更新系统串口列表
		virtual i_com_list* update_list(){return this;}

		virtual operator i_com_list*() {return static_cast<i_com_list*>(this);}
		virtual void callback(t_list_callback* cb, void* ud)
		{
			for(int i=0,c=_list.size(); i<c; i++){
				cb(ud, &_list[i]);
			}
		}

	protected:
		std::vector<T> _list;
	};

	// 串口端口列表, 继承的原因是: 端口有一个所谓的 "友好名"
	// 比如常见的: Prolific USB-to-Serial Comm Port
	// 更新到串口列表控件中时需要她们两者一起
	class c_comport : public t_com_item
	{
	public:
		c_comport(int id,const char* s)
			: t_com_item(id, s)
		{}

		std::string get_id_and_name() const;
	};

	// 串口端口容器: 要和系统取得列表, 所以重写
	class c_comport_list : public t_com_list<c_comport>
	{
	public:
		virtual i_com_list* update_list();
	};

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// 串口类
	class CComm
	{
	// UI通知者
	public:
		void set_notifier(i_notifier* noti) { _notifier = noti;	}
	private:
		i_notifier*	_notifier;

	// 发送数据包管理
	private:
		c_send_data_packet*		get_packet()	{ return _send_data.get(); }
	public:
		bool					put_packet(c_send_data_packet* psdp, bool bfront=false){
			if (is_opened()){
				if (bfront)
					_send_data.put_front(psdp);
				else
					_send_data.put(psdp);
				
				switch (psdp->type)
				{
				case csdp_type::csdp_alloc:
				case csdp_type::csdp_local:
					_data_counter.add_unsend(psdp->cb);
					break;
				}
				return true;
			}
			else{
				_notifier->msgbox(MB_ICONERROR, NULL, "串口未打开!");
				return false;
			}
		}
		c_send_data_packet*		alloc_packet(int size) { return _send_data.alloc(size); }
		void					release_packet(c_send_data_packet* psdp) { _send_data.release(psdp); }
	private:	
		c_data_packet_manager	_send_data;

	// 计数器
	public:
		c_data_counter*			counter() { return &_data_counter; }
	private:
		c_data_counter			_data_counter;

	// 数据接收器
	public:
		void add_data_recerver(i_data_receiver* receiver);
		void remove_data_recerver(i_data_receiver* receiver);
		void call_data_receivers(const unsigned char* ba, int cb);
	private:
		c_ptr_array<i_data_receiver>	_data_receivers;
		c_critical_locker				_data_receiver_lock;

	// 内部工作线程
	private:
		bool _begin_threads();
		bool _end_threads();

		struct thread_helper_context
		{
			CComm* that;
			enum e_which{
				kRead,
				kWrite,
			}which;
		};
		unsigned int thread_read();
		//void wait_read_event() { ::WaitForSingleObject(_hevent_continue_to_read, INFINITE); }
		unsigned int thread_write();
		static unsigned int __stdcall thread_helper(void* pv);
	public:
		//void resume_read_thread() { ::SetEvent(_hevent_continue_to_read); }
		//void suspend_read_thread() { ::ResetEvent(_hevent_continue_to_read); }
	private:
		HANDLE		_hthread_read;				// 读线程句柄
		HANDLE		_hthread_write;				// 写线程句柄

		HANDLE		_hevent_read_start;			// 通知读线程开始与结束的事件
		HANDLE		_hevent_read_end;			// 通知读线程开始与结束的事件

		HANDLE		_hevent_write_start;		// 通知写线程开始与结束的事件
		HANDLE		_hevent_write_end;		// 通知写线程开始与结束的事件

	// 串口配置结构体
	private:
		COMMCONFIG			_commconfig;
		COMMTIMEOUTS		_timeouts;

	// 串口设置(供外部调用)
	public:
		struct s_setting_comm{
			DWORD	baud_rate;
			BYTE	parity;
			BYTE	stopbit;
			BYTE	databit;
		};
		bool setting_comm(s_setting_comm* pssc);

	// 串口对象列表
	public:
		c_comport_list*			comports()	{ return &_comport_list; }
		t_com_list<t_com_item>*	baudrates()	{ return &_baudrate_list; }
		t_com_list<t_com_item>*	parities()	{ return &_parity_list; }
		t_com_list<t_com_item>*	stopbits()	{ return &_stopbit_list; }
		t_com_list<t_com_item>*	databits()	{ return &_databit_list; }
	private:
		c_comport_list			_comport_list;
		t_com_list<t_com_item>	_baudrate_list;
		t_com_list<t_com_item>	_parity_list;
		t_com_list<t_com_item>	_stopbit_list;
		t_com_list<t_com_item>	_databit_list;

	// 外部相关操作接口
	private:
		HANDLE		_hComPort;
	public:
		bool		open(int com_id);
		bool		close();
		HANDLE		get_handle() { return _hComPort; }
		bool		is_opened() { return !!_hComPort; }
		bool		begin_threads();
		bool		end_threads();

	public:
		CComm();
		~CComm();
	};
}

#define COMMON_MAX_LOAD_SIZE			((unsigned long)1<<20)
#define COMMON_LINE_CCH_SEND			16
#define COMMON_LINE_CCH_RECV			16
#define COMMON_SEND_BUF_SIZE			COMMON_MAX_LOAD_SIZE
#define COMMON_RECV_BUF_SIZE			0 // un-limited //(((unsigned long)1<<20)*10)
#define COMMON_INTERNAL_RECV_BUF_SIZE	((unsigned long)1<<20)
#define COMMON_READ_BUFFER_SIZE			((unsigned long)1<<20)
