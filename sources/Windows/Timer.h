#pragma once

namespace Common{
	class i_notifier;
	class i_timer
	{
	public:
		// mm timer 运行在单独的线程中, 不能直接操作UI,应SendMessage
		virtual void update_timer(int h, int m, int s) = 0;
	};

	class c_timer
	{
	public:
		c_timer()
			: _tid(0)
			, _notifier(0)
			, _timer(0)
		{}

		~c_timer();

		void start();
		void stop(bool bsetzero = false);
		void set_notifier(i_notifier* not);
		void set_timer(i_timer* tim);

	protected:
		static void __stdcall _timer_proc(UINT, UINT, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR);
		void timer_proc();


	private:
		UINT			_tid;
		unsigned char	_time_value[3];
		i_notifier*		_notifier;
		i_timer*		_timer;
	};

}
