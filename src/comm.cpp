#include "StdAfx.h"

static char* __THIS_FILE__ = __FILE__;

#include "comm.h"
#include "DataProcessor.h"

namespace Common{
	//////////////////////////////////////////////////////////////////////////
	std::string c_comport::get_id_and_name() const
	{
		char idstr[17] = {0};
		_snprintf(idstr, sizeof(idstr), "COM%-13d", _i);
		std::stringstream ss;
		ss << idstr << "\t\t" << _s;
		return std::string(ss.str());
	}

	//////////////////////////////////////////////////////////////////////////
	i_com_list* c_comport_list::update_list()
	{
		HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
		SP_DEVINFO_DATA spdata = {0};
		GUID guid = GUID_DEVINTERFACE_COMPORT;

		empty();

		hDevInfo = SetupDiGetClassDevs(&guid, 0, 0, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
		if(hDevInfo == INVALID_HANDLE_VALUE){
			return this;
		}

		spdata.cbSize = sizeof(spdata);
		for(int i=0; SetupDiEnumDeviceInfo(hDevInfo, i, &spdata); i++){
			char buff[1024] = {0};
			if(SetupDiGetDeviceRegistryProperty(hDevInfo, &spdata, SPDRP_FRIENDLYNAME, NULL, 
				PBYTE(buff), _countof(buff), NULL))
			{
				// Prolific com port (COMxx)
				char* p = strstr(buff, "(COM");
				if(p){
					int id = atoi(p + 4);
					if(p != buff) *(p-1) = '\0';
					add(c_comport(id, buff));
				}
			}
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);

		return this;
	}

	CComm::CComm()
        : _hComPort(NULL)
        , _nRead(0), _nWritten(0), _nQueued(0)
	{
		_begin_threads();
		static char* aBaudRate[]={"110","300","600","1200","2400","4800","9600","14400","19200","38400","57600","115200","128000","256000", NULL};
		static DWORD iBaudRate[]={CBR_110,CBR_300,CBR_600,CBR_1200,CBR_2400,CBR_4800,CBR_9600,CBR_14400,CBR_19200,CBR_38400,CBR_57600,CBR_115200,CBR_128000,CBR_256000};
		static char* aParity[] = {"无","奇校验","偶校验", "标记", "空格", NULL};
		static BYTE iParity[] = { NOPARITY, ODDPARITY,EVENPARITY, MARKPARITY, SPACEPARITY };
		static char* aStopBit[] = {"1位", "1.5位","2位", NULL};
		static BYTE iStopBit[] = {ONESTOPBIT,ONE5STOPBITS,TWOSTOPBITS};
		static char* aDataSize[] = {"8位","7位","6位","5位",NULL};
		static BYTE iDataSize[] = {8,7,6,5};

		for(int i=0; aBaudRate[i]; i++)
			_baudrate_list.add(c_baudrate(iBaudRate[i],aBaudRate[i], true));
		for(int i=0; aParity[i]; i++)
			_parity_list.add(t_com_item(iParity[i],aParity[i]));
		for(int i=0; aStopBit[i]; i++)
			_stopbit_list.add(t_com_item(iStopBit[i], aStopBit[i]));
		for(int i=0; aDataSize[i]; i++)
			_databit_list.add(t_com_item(iDataSize[i], aDataSize[i]));

		_timeouts.ReadIntervalTimeout = MAXDWORD;
		_timeouts.ReadTotalTimeoutMultiplier = 0;
		_timeouts.ReadTotalTimeoutConstant = 0;
		_timeouts.WriteTotalTimeoutMultiplier = 0;
		_timeouts.WriteTotalTimeoutConstant = 0;

	}


	CComm::~CComm()
	{
		_end_threads();
	}

	bool CComm::open(int com_id)
	{
		if (is_opened()){
			SMART_ASSERT("com was opened!" && 0).Fatal();
			return false;
		}

		char str[64];
		sprintf(str, "\\\\.\\COM%d", com_id);
		_hComPort = ::CreateFile(str, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (_hComPort == INVALID_HANDLE_VALUE){
			_hComPort = NULL;
			DWORD dwErr = ::GetLastError();
            // TODO
			//_notifier->msgerr();
			if (dwErr == ERROR_FILE_NOT_FOUND){
				//TODO
			}
			return false;
		}


		return true;
	}

	bool CComm::close()
	{
		SMART_ENSURE(::CloseHandle(_hComPort), != 0).Fatal();
		_hComPort = NULL;
        reset_counter();
		_send_data.empty();

		return true;
	}

	bool CComm::begin_threads()
	{
		::ResetEvent(_thread_read.hEventToExit);
		::ResetEvent(_thread_write.hEventToExit);
		::ResetEvent(_thread_event.hEventToExit);
		
		::SetEvent(_thread_read.hEventToBegin);
		::SetEvent(_thread_write.hEventToBegin);
		::SetEvent(_thread_event.hEventToBegin);

		return true;
	}

	bool CComm::end_threads()
	{
		::ResetEvent(_thread_read.hEventToBegin);
		::ResetEvent(_thread_write.hEventToBegin);
		::ResetEvent(_thread_event.hEventToBegin);

		::SetEvent(_thread_read.hEventToExit);
		::SetEvent(_thread_write.hEventToExit);
		::SetEvent(_thread_event.hEventToExit);
		
		// 在读写线程退出之前, 两个end均为激发状态
		// 必须等到两个线程均退出工作状态才能有其它操作
		debug_out(("等待 [读线程] 结束...\n"));
		while (::WaitForSingleObject(_thread_read.hEventToExit, 0) == WAIT_OBJECT_0);
		debug_out(("等待 [写线程] 结束...\n"));
		while (::WaitForSingleObject(_thread_write.hEventToExit, 0) == WAIT_OBJECT_0);
		debug_out(("等待 [事件线程] 结束...\n"));
		while (::WaitForSingleObject(_thread_event.hEventToExit, 0) == WAIT_OBJECT_0);

		return true;
	}

	unsigned int __stdcall CComm::thread_helper(void* pv)
	{
		auto pctx = reinterpret_cast<thread_helper_context*>(pv);
		auto comm = pctx->that;
		auto which = pctx->which;

		delete pctx;

		switch (which)
		{
		case thread_helper_context::e_which::kEvent:
			return comm->thread_event();
		case thread_helper_context::e_which::kRead:
			return comm->thread_read();
		case thread_helper_context::e_which::kWrite:
			return comm->thread_write();
		default:
			SMART_ASSERT(0 && "unknown thread").Fatal();
			return 1;
		}
	}

	unsigned int CComm::thread_event()
	{
		BOOL bRet;
		DWORD dw,dw2;

	_wait_for_work:
		debug_out(("[事件线程] 就绪!\n"));
		dw = ::WaitForSingleObject(_thread_event.hEventToBegin, INFINITE);
		SMART_ASSERT(dw == WAIT_OBJECT_0)(dw).Fatal();

		debug_out(("[事件线程] 开始工作...\n"));
		if (!is_opened()){
			debug_out(("[事件线程] 没有工作, 退出中...\n"));
			::SetEvent(_thread_event.hEventToExit);
			return 0;
		}

		c_overlapped o(true, false);

	_wait_again:
		DWORD dwEvent = 0;
		::ResetEvent(o.hEvent);
		dw = ::WaitCommEvent(_hComPort, &dwEvent, &o);
		if (dw != FALSE){
			_event_listener.call_listeners(dwEvent);
			goto _wait_again;
		}
		else{ // I/O is pending
			if (::GetLastError() == ERROR_IO_PENDING){
				HANDLE handles[2];
				handles[0] = _thread_event.hEventToExit;
				handles[1] = o.hEvent;

				switch (::WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE))
				{
				case WAIT_FAILED:
                    // TODO
					//_notifier->msgerr("[事件线程::Wait失败]");
					goto _restart;
					break;
				case WAIT_OBJECT_0 + 0:
					debug_out(("[事件线程] 收到退出事件!\n"));
					goto _restart;
					break;
				case WAIT_OBJECT_0 + 1:
					bRet = ::GetOverlappedResult(_hComPort, &o, &dw2, FALSE);
					if (bRet == FALSE){
                        // TODO
						//_notifier->msgerr("[事件线程::Wait失败]");
						goto _restart;
					}
					else{
						_event_listener.call_listeners(dwEvent); // uses dwEvent, not dw2
						goto _wait_again;
					}
					break;
				}
			}
			else{
				//TODO
				// _notifier->msgerr("[事件线程]::GetLastError() != ERROR_IO_PENDING\n\n");
			}
		}

	_restart:
		if (!::CancelIo(_hComPort)){

		}

		::WaitForSingleObject(_thread_event.hEventToExit, INFINITE);
		::ResetEvent(_thread_event.hEventToExit);

		goto _wait_for_work;
	}

	unsigned int CComm::thread_write()
	{
		BOOL bRet;
		DWORD dw;

		c_event_event_listener listener;

	_wait_for_work:
		debug_out(("[写线程] 就绪\n"));
		dw = ::WaitForSingleObject(_thread_write.hEventToBegin, INFINITE);
		SMART_ASSERT(dw == WAIT_OBJECT_0)(dw).Fatal();
		
		debug_out(("[写线程] 开始工作...\n"));
		if (!is_opened()){
			debug_out(("[写线程] 没有工作, 退出中...\n"));
			::SetEvent(_thread_write.hEventToExit);
			return 0;
		}

		c_overlapped overlap(false, false);
		
		_event_listener.add_listener(listener, EV_TXEMPTY);

	_get_packet:
		debug_out(("[写线程] 取数据包中...\n"));
		c_send_data_packet* psdp = _send_data.get();
		if (psdp->type == csdp_type::csdp_alloc || psdp->type == csdp_type::csdp_local){
			debug_out(("[写线程] 取得一个发送数据包, 长度为 %d 字节\n", psdp->cb));

			DWORD	nWritten = 0;		// 写操作一次写入的长度
			int		nWrittenData;		// 当前循环总共写入长度

			for (nWrittenData = 0; nWrittenData < psdp->cb;){
				bRet = ::WriteFile(_hComPort, &psdp->data[0] + nWrittenData, psdp->cb - nWrittenData, NULL, &overlap);
				if (bRet != FALSE){ // I/O is completed
					bRet = ::GetOverlappedResult(_hComPort, &overlap, &nWritten, FALSE);
					if (bRet){
						debug_out(("[写线程] I/O completed immediately, bytes : %d\n", nWritten));
					}
					else{
                        //TODO
						//_notifier->msgerr("[写线程] GetOverlappedResult失败(I/O completed)!\n");
						goto _restart;
					}
				}
				else{ // I/O is pending						
					if (::GetLastError() == ERROR_IO_PENDING){
						HANDLE handles[2];
						handles[0] = _thread_write.hEventToExit;
						handles[1] = listener.hEvent;

						switch (::WaitForMultipleObjects(_countof(handles), &handles[0], FALSE, INFINITE))
						{
						case WAIT_FAILED:
                            //TODO
							//_notifier->msgerr("[写线程] Wait失败!\n");
							goto _restart;
							break;
						case WAIT_OBJECT_0 + 0: // now we exit
							debug_out(("[写线程] 收到退出事件!\n"));
							goto _restart;
							break;
						case WAIT_OBJECT_0 + 1: // the I/O operation is now completed
							bRet = ::GetOverlappedResult(_hComPort, &overlap, &nWritten, FALSE);
							if (bRet){
								debug_out(("[写线程] 写入 %d 个字节!\n", nWritten));
							}
							else{
                                //TODO
								//_notifier->msgerr("[写线程] GetOverlappedResult失败(I/O pending)!\n");
								goto _restart;
							}
							break;
						}
					}
					else{
                        //TODO
						//_notifier->msgerr("[写线程] ::GetLastError() != ERROR_IO_PENDING");
						goto _restart;
					}
				}

				nWrittenData += nWritten;
                update_counter(0, nWritten, -(int)nWritten);
			}
			_send_data.release(psdp);
			goto _get_packet;
		}
		else if (psdp->type == csdp_type::csdp_exit){
			debug_out(("[写线程] 收到退出事件!\n"));
			_send_data.release(psdp);
			goto _restart;
		}

	_restart:
		if (!::CancelIo(_hComPort)){

		}

		_event_listener.remove_listener(listener);
		listener.reset();

		// Do just like the thread_read do.
		::WaitForSingleObject(_thread_write.hEventToExit, INFINITE);
		::ResetEvent(_thread_write.hEventToExit);

		goto _wait_for_work;
	}

	unsigned int CComm::thread_read()
	{
		BOOL bRet;
		DWORD dw;

		c_event_event_listener listener;

		const int kReadBufSize = 1 << 20;
		unsigned char* block_data = NULL;
		block_data = new unsigned char[kReadBufSize];

	_wait_for_work:
		debug_out(("[读线程] 就绪\n"));
		dw = ::WaitForSingleObject(_thread_read.hEventToBegin, INFINITE);
		SMART_ASSERT(dw == WAIT_OBJECT_0)(dw).Fatal();

		debug_out(("[读线程] 开始工作...\n"));
		if (!is_opened()){
			debug_out(("[读线程] 没有工作, 退出中...\n"));
			delete[] block_data;
			::SetEvent(_thread_read.hEventToExit);
			return 0;
		}

		c_overlapped overlap(false, false);

		_event_listener.add_listener(listener, EV_RXCHAR);


		HANDLE handles[2];
		handles[0] = _thread_read.hEventToExit;
		handles[1] = listener.hEvent;

	_get_packet:
		switch (::WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE))
		{
		case WAIT_FAILED:
            //TODO
			//_notifier->msgerr("[读线程] Wait失败!\n");
			goto _restart;
		case WAIT_OBJECT_0 + 0:
			debug_out(("[读线程] 收到退出事件!\n"));
			goto _restart;
		case WAIT_OBJECT_0 + 1:
			break;
		}

		DWORD nBytesToRead, nRead, nTotalRead;
		DWORD	comerr;
		COMSTAT	comsta;
		// for some reasons, such as comport has been removed
		if (!::ClearCommError(_hComPort, &comerr, &comsta)){
            //TODO
			//_notifier->msgerr("ClearCommError()");
			goto _restart;
		}

		nBytesToRead = comsta.cbInQue;
		if (nBytesToRead == 0) 
			nBytesToRead++; // would never happen

		if (nBytesToRead > kReadBufSize)
			nBytesToRead = kReadBufSize;

		for (nTotalRead = 0; nTotalRead < nBytesToRead;){
			bRet = ::ReadFile(_hComPort, block_data + nTotalRead, nBytesToRead - nTotalRead, &nRead, &overlap);
			if (bRet != FALSE){
				bRet = ::GetOverlappedResult(_hComPort, &overlap, &nRead, FALSE);
				if (bRet) {
					debug_out(("[读线程] 读取 %d 字节, bRet==TRUE, nBytesToRead: %d\n", nRead, nBytesToRead));
				}
				else{
                    //TODO
					//_notifier->msgerr("[写线程] GetOverlappedResult失败!\n");
					goto _restart;
				}
			}
			else{
				if (::GetLastError() == ERROR_IO_PENDING){
					HANDLE handles[2];
					handles[0] = _thread_read.hEventToExit;
					handles[1] = overlap.hEvent;

					switch (::WaitForMultipleObjects(_countof(handles), &handles[0], FALSE, INFINITE))
					{
					case WAIT_FAILED:
						debug_out(("[读线程] 等待失败!\n"));
						goto _restart;
					case WAIT_OBJECT_0 + 0:
						debug_out(("[读线程] 收到退出事件!\n"));
						goto _restart;
					case WAIT_OBJECT_0 + 1:
						bRet = ::GetOverlappedResult(_hComPort, &overlap, &nRead, FALSE);
						if (bRet){
							debug_out(("[读线程] 读取 %d 字节, bRet==FALSE\n", nRead));
						}
						else{
                            //TODO
							//_notifier->msgerr("[读线程] GetOverlappedResult失败!\n");
							goto _restart;
						}
						break;
					}
				}
				else{
                    //TODO
					//_notifier->msgerr("[读线程] ::GetLastError() != ERROR_IO_PENDING");
					goto _restart;
				}
			}

			if (nRead > 0){
				nTotalRead += nRead;
                update_counter(nRead, 0, 0);
			}
			else{
				nBytesToRead--;
			}
		}
		call_data_receivers(block_data, nBytesToRead);
		goto _get_packet;

	_restart:
		if (!::CancelIo(_hComPort)){

		}

		// Sometimes we got here not because of we've got a exit signal
		// Maybe something wrong
		// And if something wrong, the following handle is still non-signal.
		// The main thread notify this thread to exit by signaling the event and then wait
		// this thread Reset it, since the event is a Manual reset event handle.
		// So, let's wait whatever the current signal-state the event is, just before the
		// main thread  really want we do that.
		::WaitForSingleObject(_thread_read.hEventToExit, INFINITE);
		::ResetEvent(_thread_read.hEventToExit);

		goto _wait_for_work;
	}

	//////////////////////////////////////////////////////////////////////////
	void CComm::call_data_receivers(const unsigned char* ba, int cb)
	{
		_data_receiver_lock.lock();
		for (int i = 0; i < _data_receivers.size(); i++){
			_data_receivers[i]->receive(ba, cb);
		}
		_data_receiver_lock.unlock();
	}

	void CComm::remove_data_receiver(i_data_receiver* receiver)
	{
		_data_receiver_lock.lock();
		_data_receivers.remove(receiver);
		_data_receiver_lock.unlock();

	}

	void CComm::add_data_receiver(i_data_receiver* receiver)
	{
		_data_receiver_lock.lock();
		_data_receivers.add(receiver);
		_data_receiver_lock.unlock();
	}
	//////////////////////////////////////////////////////////////////////////

	bool CComm::setting_comm(s_setting_comm* pssc)
	{
		SMART_ASSERT(is_opened()).Fatal();

		if (!::GetCommState(get_handle(), &_dcb)){
            //TODO
			//_notifier->msgerr("GetCommState()错误");
			return false;
		}

		_dcb.fBinary = TRUE;
		_dcb.BaudRate = pssc->baud_rate;
		_dcb.fParity = pssc->parity == NOPARITY ? FALSE : TRUE;
		_dcb.Parity = pssc->parity;
		_dcb.ByteSize = pssc->databit;
		_dcb.StopBits = pssc->stopbit;

		if (!::SetCommState(_hComPort, &_dcb)){
            //TODO
			//_notifier->msgerr("SetCommState()错误");
			return false;
		}

		if (!::SetCommMask(get_handle(), 
			EV_RXCHAR|EV_RXFLAG|EV_TXEMPTY
			| EV_CTS | EV_DSR | EV_RLSD
			| EV_BREAK | EV_ERR
			| EV_RING
			| EV_PERR | EV_RX80FULL))
		{
            //TODO
			//_notifier->msgerr("SetCommMask()错误");

			return false;
		}
		if (!::SetCommTimeouts(get_handle(), &_timeouts)){
            //TODO
			//_notifier->msgerr("设置串口超时错误");
			return false;
		}

		PurgeComm(_hComPort, PURGE_TXCLEAR | PURGE_TXABORT);
		PurgeComm(_hComPort, PURGE_RXCLEAR | PURGE_RXABORT);

		return true;
	}

	bool CComm::_begin_threads()
	{
		thread_helper_context* pctx = nullptr;

		// 开启读线程
		_thread_read.hEventToBegin = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		_thread_read.hEventToExit = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

		pctx = new thread_helper_context;
		pctx->that = this;
		pctx->which = thread_helper_context::e_which::kRead;
		_thread_read.hThread = (HANDLE)::_beginthreadex(nullptr, 0, thread_helper, pctx, 0, nullptr);

		if (!_thread_read.hEventToBegin || !_thread_read.hEventToExit || !_thread_read.hThread){
			::MessageBox(NULL, "应用程序初始化失败, 即将退出!", NULL, MB_ICONHAND);
			::exit(1);
		}

		// 开启写线程
		_thread_write.hEventToBegin = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		_thread_write.hEventToExit = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

		pctx = new thread_helper_context;
		pctx->that = this;
		pctx->which = thread_helper_context::e_which::kWrite;
		_thread_write.hThread = (HANDLE)::_beginthreadex(nullptr, 0, thread_helper, pctx, 0, nullptr);

		if (!_thread_write.hEventToBegin || !_thread_write.hEventToExit || !_thread_write.hThread){
			::MessageBox(NULL, "应用程序初始化失败, 即将退出!", NULL, MB_ICONHAND);
			::exit(1);
		}

		// 开启事件线程
		_thread_event.hEventToBegin = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		_thread_event.hEventToExit = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

		pctx = new thread_helper_context;
		pctx->that = this;
		pctx->which = thread_helper_context::e_which::kEvent;
		_thread_event.hThread = (HANDLE)::_beginthreadex(nullptr, 0, thread_helper, pctx, 0, nullptr);

		if (!_thread_event.hEventToBegin || !_thread_event.hEventToExit || !_thread_event.hThread){
			::MessageBox(NULL, "应用程序初始化失败, 即将退出!", NULL, MB_ICONHAND);
			::exit(1);
		}

		return true;
	}

	bool CComm::_end_threads()
	{
		SMART_ASSERT(is_opened() == false).Fatal();

		// 由线程在退出之前设置并让当前线程等待他们的结束
		::ResetEvent(_thread_read.hEventToExit);
		::ResetEvent(_thread_write.hEventToExit);
		::ResetEvent(_thread_event.hEventToExit);

		// 此时串口是关闭的, 收到此事件即准备退出线程
		::SetEvent(_thread_read.hEventToBegin);
		::SetEvent(_thread_write.hEventToBegin);
		::SetEvent(_thread_event.hEventToBegin);

		// 等待线程完全退出
		::WaitForSingleObject(_thread_read.hEventToExit, INFINITE);
		::WaitForSingleObject(_thread_write.hEventToExit, INFINITE);
		::WaitForSingleObject(_thread_event.hEventToExit, INFINITE);

		::CloseHandle(_thread_read.hEventToBegin);
		::CloseHandle(_thread_read.hEventToExit);
		::CloseHandle(_thread_write.hEventToBegin);
		::CloseHandle(_thread_write.hEventToExit); 
		::CloseHandle(_thread_event.hEventToBegin);
		::CloseHandle(_thread_event.hEventToExit);

		::CloseHandle(_thread_read.hThread);
		::CloseHandle(_thread_write.hThread);
		::CloseHandle(_thread_event.hThread);

		return false;
	}

    void CComm::update_counter(int nRead, int nWritten, int nQueued) {
        if(nRead)       InterlockedExchangeAdd(&_nRead, nRead);
        if(nWritten)    InterlockedExchangeAdd(&_nWritten, nWritten);
        if(nQueued)     InterlockedExchangeAdd(&_nQueued, nQueued);

        if(nRead || nWritten || nQueued) {
            auto cmd = new Command_UpdateCounter;
            cmd->nRead = nRead;
            cmd->nWritten = nWritten;
            cmd->nQueued = nQueued;
            _commands.push_back(cmd);
        }
    }

    void CComm::reset_counter(bool r, bool w, bool q) {
        if(r) InterlockedExchange(&_nRead, 0);
        if(w) InterlockedExchange(&_nWritten, 0);
        if(q) InterlockedExchange(&_nQueued, 0);

        auto cmd = new Command_UpdateCounter;
        cmd->nRead = _nRead;
        cmd->nWritten = _nWritten;
        cmd->nQueued = _nQueued;
        _commands.push_back(cmd);
    }

    void CComm::get_counter(int* pRead, int* pWritten, int* pQueued) {
        *pRead = _nRead;
        *pWritten = _nWritten;
        *pQueued = _nQueued;
    }

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	c_data_packet_manager::c_data_packet_manager()
		: _hEvent(0)
	{
		_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		SMART_ASSERT(_hEvent != NULL).Fatal();
		
		list_init(&_list);
		for (int i = 0; i < sizeof(_data) / sizeof(_data[0]); i++)
			_data[i].used = false;
	}

	c_data_packet_manager::~c_data_packet_manager()
	{
		SMART_ASSERT(list_is_empty(&_list)).Fatal();
		::CloseHandle(_hEvent);
	}

	c_send_data_packet* c_data_packet_manager::alloc(int size)
	{
		SMART_ASSERT(size >= 0)(size).Fatal();
		_lock.lock();

		c_send_data_packet* psdp = NULL;

		if (size <= csdp_def_size){
			for (int i = 0; i < sizeof(_data) / sizeof(_data[0]); i++){
				if (_data[i].used == false){
					psdp = (c_send_data_packet*)&_data[i];
					break;
				}
			}
			if (psdp != NULL){
				psdp->used = true;
				psdp->type = csdp_type::csdp_local;
				psdp->cb = size;
				goto _exit;
			}
			// no left
		}

		while (psdp == NULL){
			psdp = (c_send_data_packet*)GET_MEM(sizeof(c_send_data_packet) + size);
		}
		psdp->type = csdp_type::csdp_alloc;
		psdp->used = true;
		psdp->cb = size;
		goto _exit;

	_exit:
		_lock.unlock();
		return psdp;
	}

	void c_data_packet_manager::release(c_send_data_packet* psdp)
	{
		SMART_ASSERT(psdp != NULL).Fatal();

		switch (psdp->type)
		{
		case csdp_type::csdp_alloc:
			memory.free((void**)&psdp, "");
			break;
		case csdp_type::csdp_local:
		case csdp_type::csdp_exit:
			_lock.lock();
			psdp->used = false;
			_lock.unlock();
			break;
		default:
			SMART_ASSERT(0).Fatal();
		}
	}

	void c_data_packet_manager::put(c_send_data_packet* psdp)
	{
		_lock.lock();
		list_insert_tail(&_list, &psdp->_list_entry);
		_lock.unlock();
		::SetEvent(_hEvent); // singal get() proc
	}

	c_send_data_packet* c_data_packet_manager::get()
	{
		c_send_data_packet* psdp = NULL;

		for (;;){ // 无限等待, 直到收到一个数据包
			_lock.lock();
			list_s* pls = list_remove_head(&_list);
			_lock.unlock();

			if (pls != NULL){
				psdp = list_data(pls, c_send_data_packet, _list_entry);
				return psdp;
			}
			else{
				::WaitForSingleObject(_hEvent, INFINITE);
			}
		}
	}

	void c_data_packet_manager::put_front(c_send_data_packet* psdp)
	{
		_lock.lock();
		list_insert_head(&_list, &psdp->_list_entry);
		_lock.unlock();
		::SetEvent(_hEvent);
	}

	void c_data_packet_manager::empty()
	{
		while (!list_is_empty(&_list)){
			list_s* p = list_remove_head(&_list);
			c_send_data_packet* psdp = list_data(p, c_send_data_packet, _list_entry);
			release(psdp);
		}
	}

	c_send_data_packet* c_data_packet_manager::query_head()
	{
		c_send_data_packet* psdp = NULL;
		_lock.lock();
		if (list_is_empty(&_list)){
			psdp = NULL;
		}
		else{
			psdp = list_data(_list.next, c_send_data_packet, _list_entry);
		}
		_lock.unlock();
		return psdp;
	}

}
