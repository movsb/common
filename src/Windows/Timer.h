#pragma once

namespace Common{
	class i_notifier;
	class i_timer
	{
	public:
		// mm timer 运行在单独的线程中, 不能直接操作UI,应SendMessage
		virtual void update_timer(int h, int m, int s) = 0;
	};

	class i_timer_period
	{
	public:
		// 周期通知定时器接口
		virtual void update_timer_period() = 0;
	};

	class c_timer
	{
	public:
		c_timer()
			: _tid(0)
			, _notifier(0)
			, _timer(0)
			, _period_timer(0)
		{}

		~c_timer();

		void start();
		void stop(bool bsetzero = false);
		void set_notifier(i_notifier* not);
		void set_timer(i_timer* tim);
		void set_period(int period);
		int get_period() const;
		void set_period_timer(i_timer_period* tim);
        bool is_running() const;

	protected:
		static void __stdcall _timer_proc(UINT, UINT, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR);
		void timer_proc();


	private:
		int				_period;
		UINT			_tid;
		unsigned char	_time_value[3];
		i_notifier*		_notifier;
		i_timer*		_timer;
		i_timer_period*	_period_timer;
	};

}
