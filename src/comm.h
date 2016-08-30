#pragma once

namespace Common {

    struct ICommandNotifier
    {
        virtual void OnCommand() = 0;
    };

    enum class CommandType
    {
        kNull,
		kCommEvent,
        kErrorMessage,
        kUpdateCounter,
		kReceiveData,
    };

    struct Command
    {
        Command(CommandType type_) 
            : type(type_) 
        {}

        CommandType type;
    };

	struct Command_CommEvent : Command
	{
		Command_CommEvent() : Command(CommandType::kCommEvent) {}

		DWORD event;
	};

    struct Command_ErrorMessage : Command
    {
        Command_ErrorMessage() : Command(CommandType::kErrorMessage) {}

        int code;
        std::string what;
    };

    struct Command_UpdateCounter : Command
    {
        Command_UpdateCounter() : Command(CommandType::kUpdateCounter) {}

        int nRead;
        int nWritten;
        int nQueued;
    };

	struct Command_ReceiveData : Command
	{
		Command_ReceiveData() : Command(CommandType::kReceiveData) {}

		std::string data;
	};

    class CommandQueue
    {
    public:
        void set_notifier(ICommandNotifier* p) {
            _notifier = p;
        }

        void notify() {
            _notifier->OnCommand();
        }

        bool empty() {
            c_lock_guard _guard(_lock);
            return _commands.empty();
        }

        void clear() {
            c_lock_guard _guard(_lock);

            for(auto& p : _commands)
                delete p;

            _commands.clear();
        }

        void push_back(Command* cmd) {
            c_lock_guard _guard(_lock);
            _commands.push_back(cmd);
            notify();
        }

        void push_front(Command* cmd) {
            c_lock_guard _guard(_lock);
            _commands.push_front(cmd);
            notify();
        }

        Command* pop_back() {
            c_lock_guard _guard(_lock);
            auto p = _commands.back();
            _commands.pop_back();
            return p;
        }

        Command* pop_front() {
            c_lock_guard _guard(_lock);
            auto p = _commands.front();
            _commands.pop_front();
            return p;
        }

        Command* try_pop_front() {
            c_lock_guard _guard(_lock);
            Command* pCmd;
            if(!_commands.empty()) {
                pCmd = _commands.front();
                _commands.pop_front();
            }
            else {
                pCmd = nullptr;
            }
            return pCmd;
        }

    private:
        c_critical_locker       _lock;
        std::list<Command*>     _commands;
        ICommandNotifier*       _notifier;
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

#pragma warning(push)
#pragma warning(disable:4200)	// nonstandard extension used : zero-sized array in struct/union
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
#pragma warning(pop)

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

	// 串口事件监听器接口
	class i_com_event_listener
	{
	public:
		virtual void do_event(DWORD evt) = 0;
	};

	// do_event的过程一定不要长时间不返回, 所以写了个基于事件的监听器
	class c_event_event_listener : public i_com_event_listener
	{
	public:
		operator i_com_event_listener*() {
			return this;
		}
		virtual void do_event(DWORD evt) override
		{
			event = evt;
			::SetEvent(hEvent);
		}

	public:
		c_event_event_listener()
		{
			hEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		}
		~c_event_event_listener()
		{
			::CloseHandle(hEvent);
		}

		void reset()
		{
			::ResetEvent(hEvent);
		}


	public:
		DWORD	event;
		HANDLE	hEvent;
	};

	// 串口事件监听器接口 管理器
	class c_com_event_listener
	{
		struct item{
			item(i_com_event_listener* p, DWORD _mask)
				: listener(p)
				, mask(_mask)
			{}

			i_com_event_listener* listener;
			DWORD mask;
		};
	public:
		void call_listeners(DWORD dwEvt){
			_lock.lock();
			for (auto& item : _listeners){
				if (dwEvt & item.mask){
					item.listener->do_event(dwEvt);
				}
			}
			_lock.unlock();
		}
		void add_listener(i_com_event_listener* pcel, DWORD mask){
			_lock.lock();
			_listeners.push_back(item(pcel, mask));
			_lock.unlock();
		}
		void remove_listener(i_com_event_listener* pcel){
			_lock.lock();
			for (auto it = _listeners.begin(); it != _listeners.end(); it++){
				if (it->listener == pcel){
					_listeners.erase(it);
					break;
				}
			}
			_lock.unlock();
		}

	protected:
		c_critical_locker	_lock;
		std::vector<item>	_listeners;
	};

	// 串口类
	class CComm
	{
	public:
		CComm();
		~CComm();

	// 命令相关
	private:
        CommandQueue _commands;
    public:
        void set_notifier(ICommandNotifier* notifier) {
            _commands.set_notifier(notifier);
        }

        Command* get_command() {
            return _commands.try_pop_front();
        }

	// 杂项方法
	private:
        // 基于当前系统错误码产生错误消息到命令队列
        void system_error(const std::string& prefix = "");

	// 发送数据包管理
	private:
		c_send_data_packet*		get_packet()	{ return _send_data.get(); }
	public:
		bool					put_packet(c_send_data_packet* psdp, bool bfront=false, bool bsilent = false){
			if (is_opened()){
				if (bfront)
					_send_data.put_front(psdp);
				else
					_send_data.put(psdp);
				
				switch (psdp->type)
				{
				case csdp_type::csdp_alloc:
				case csdp_type::csdp_local:
                    update_counter(0, 0, psdp->cb);
					break;
				}
				return true;
			}
			else{
                // TODO
				//if (!bsilent)
				//	_notifier->msgbox(MB_ICONERROR, NULL, "串口未打开!");
				release_packet(psdp);
				return false;
			}
		}
		c_send_data_packet*		alloc_packet(int size) { return _send_data.alloc(size); }
		void					release_packet(c_send_data_packet* psdp) { _send_data.release(psdp); }
		void					empty_packet_list() { _send_data.empty(); }
	private:	
		c_data_packet_manager	_send_data;

	// 计数器
	public:
        void get_counter(int* pRead, int* pWritten, int* pQueued);
        void reset_counter(bool r = true, bool w = true, bool q = true);
    private:
        void update_counter(int nRead, int nWritten, int nQueued);
        volatile long _nRead, _nWritten, _nQueued;

	// 事件监听器
	private:
		c_com_event_listener	_event_listener;

	// 内部工作线程
	private:
		bool _begin_threads();
		bool _end_threads();

		class c_overlapped : public OVERLAPPED
		{
		public:
			c_overlapped(bool manual, bool sigaled)
			{
				Internal = 0;
				InternalHigh = 0;
				Offset = 0;
				OffsetHigh = 0;
				hEvent = ::CreateEvent(nullptr, manual, sigaled ? TRUE : FALSE, nullptr);
			}
			~c_overlapped()
			{
				::CloseHandle(hEvent);
			}
		};

		struct thread_helper_context
		{
			CComm* that;
			enum class e_which{
				kEvent,
				kRead,
				kWrite,
			};
			e_which which;
		};
		struct thread_state{
			HANDLE hThread;
			HANDLE hEventToBegin;
			HANDLE hEventToExit;
		};
		unsigned int thread_event();
		unsigned int thread_read();
		unsigned int thread_write();
		static unsigned int __stdcall thread_helper(void* pv);

		thread_state	_thread_read;
		thread_state	_thread_write;
		thread_state	_thread_event;


	public:
		struct s_setting_comm{
			DWORD	baud_rate;
			BYTE	parity;
			BYTE	stopbit;
			BYTE	databit;
		};
		bool setting_comm(s_setting_comm* pssc);

	public:
		bool		open(int com_id);
		bool		close();
		HANDLE		get_handle() { return _hComPort; }
		bool		is_opened() { return !!_hComPort; }

	// 发送数据
	public:
		void		write(const void* data, unsigned int cb = ~0);
		void		write(const std::string& data);
		bool		begin_threads();
		bool		end_threads();

	private:
		HANDLE		_hComPort;
	};
}
