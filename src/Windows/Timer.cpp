#include "stdafx.h"

#pragma comment(lib,"winmm")

namespace Common{
	void c_timer::start()
	{
		SMART_ASSERT(_tid == 0).Fatal();

		if(_timer) _timer->update_timer(0,0,0);
		_time_value[0] = _time_value[1] = _time_value[2] = 0;

		_tid = ::timeSetEvent(_period, 0, _timer_proc, DWORD_PTR(this), TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
		SMART_ASSERT(_tid != 0).Warning();
		if (_tid == 0 && _notifier)
			_notifier->msgerr("创建系统定时器失败");
	}

	void c_timer::stop(bool bsetzero)
	{
		if (_tid){
			::timeKillEvent(_tid);
			_tid = 0;
			if (bsetzero && _timer)
				_timer->update_timer(0, 0, 0);
		}
	}

	void c_timer::set_notifier(i_notifier* not)
	{
		_notifier = not;
	}

	void c_timer::set_timer(i_timer* tim)
	{
		_timer = tim;
	}

	void __stdcall c_timer::_timer_proc(UINT, UINT, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR)
	{
		return reinterpret_cast<c_timer*>(dwUser)->timer_proc();
	}

	void c_timer::timer_proc()
	{
		if (_timer){
			if (++_time_value[0] == 60){
				_time_value[0] = 0;
				if (++_time_value[1] == 60){
					_time_value[1] = 0;
					if (++_time_value[2] == 24){
						_time_value[2] = 0;
					}
				}
			}
			_timer->update_timer(
				_time_value[2], _time_value[1], _time_value[0]);
		}

		if (_period_timer){
			_period_timer->update_timer_period();
		}
	}

	c_timer::~c_timer()
	{
		if (_tid != 0){
			stop();
		}
	}

	void c_timer::set_period_timer(i_timer_period* tim)
	{
		_period_timer = tim;
	}

    bool c_timer::is_running() const {
        return _tid != 0;
    }

	void c_timer::set_period(int period)
	{
		SMART_ASSERT(period > 0)(period).Fatal();
		_period = period;
	}

	int c_timer::get_period() const
	{
		return _period;
	}

}
