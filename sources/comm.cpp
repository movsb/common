#include "StdAfx.h"

static char* __THIS_FILE__ = __FILE__;


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

		empty();

		hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
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
		: _notifier(NULL)
		, _hComPort(NULL)
		, _hthread_read(0)
		, _hthread_write(0)
		, _hevent_continue_to_read(0)

	{
		static char* aBaudRate[]={"110","300","600","1200","2400","4800","9600","14400","19200","38400","57600","115200","128000","256000", NULL};
		static DWORD iBaudRate[]={CBR_110,CBR_300,CBR_600,CBR_1200,CBR_2400,CBR_4800,CBR_9600,CBR_14400,CBR_19200,CBR_38400,CBR_57600,CBR_115200,CBR_128000,CBR_256000};
		static char* aParity[] = {"无","偶校验","奇校验", "标记", "空格", NULL};
		static BYTE iParity[] = {NOPARITY,EVENPARITY,ODDPARITY,MARKPARITY,SPACEPARITY};
		static char* aStopBit[] = {"1位", "1.5位","2位", NULL};
		static BYTE iStopBit[] = {ONESTOPBIT,ONE5STOPBITS,TWOSTOPBITS};
		static char* aDataSize[] = {"8位","7位","6位","5位",NULL};
		static BYTE iDataSize[] = {8,7,6,5};

		for(int i=0; aBaudRate[i]; i++)
			_baudrate_list.add(t_com_item(iBaudRate[i],aBaudRate[i]));
		for(int i=0; aParity[i]; i++)
			_parity_list.add(t_com_item(iParity[i],aParity[i]));
		for(int i=0; aStopBit[i]; i++)
			_stopbit_list.add(t_com_item(iStopBit[i], aStopBit[i]));
		for(int i=0; aDataSize[i]; i++)
			_databit_list.add(t_com_item(iDataSize[i], aDataSize[i]));

		_timeouts.ReadIntervalTimeout = 0;
		_timeouts.ReadTotalTimeoutMultiplier = 1;
		_timeouts.ReadTotalTimeoutConstant = 0;
		_timeouts.WriteTotalTimeoutMultiplier = 1;
		_timeouts.WriteTotalTimeoutConstant = 0;


		::memset(&_commconfig, 0, sizeof(_commconfig));
	}


	CComm::~CComm()
	{

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
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL);
		if (_hComPort == INVALID_HANDLE_VALUE){
			_hComPort = NULL;
			DWORD dwErr = ::GetLastError();
			_notifier->msgerr();
			if (dwErr == ERROR_FILE_NOT_FOUND){
				//TODO
			}
			return false;
		}


		return true;
	}

	bool CComm::close()
	{
		end_threads();
		_hComPort = NULL;
		_hevent_continue_to_read = NULL;
		_hthread_read = NULL;
		_hthread_write = NULL;

		_data_counter.reset_all();
		_data_counter.call_updater();

		_send_data.empty();

		return true;
	}

	bool CComm::begin_threads()
	{
		SMART_ASSERT(!_hthread_read && !_hthread_write && !_hevent_continue_to_read).Fatal();

		_hevent_continue_to_read = ::CreateEvent(NULL, TRUE, TRUE, NULL);
		SMART_ASSERT(_hevent_continue_to_read != NULL).Fatal();

		thread_helper_context* ctx = NULL;;

		ctx = new thread_helper_context;
		ctx->that = this;
		ctx->which = thread_helper_context::kRead;
		_hthread_read = (HANDLE)_beginthreadex(NULL, 0, thread_helper, ctx, 0, NULL);

		ctx = new thread_helper_context;
		ctx->that = this;
		ctx->which = thread_helper_context::kWrite;
		_hthread_write = (HANDLE)_beginthreadex(NULL, 0, thread_helper, ctx, 0, NULL);

		SMART_ASSERT(_hthread_read && _hthread_write).Fatal();

		return true;
	}

	bool CComm::end_threads()
	{

		return 0;
	}

	unsigned int __stdcall CComm::thread_helper(void* pv)
	{
		thread_helper_context* pctx = (thread_helper_context*)pv;
		CComm* comm = pctx->that;

		switch (pctx->which)
		{
		case thread_helper_context::e_which::kRead:
			delete pctx;
			return comm->thread_read();
		case thread_helper_context::e_which::kWrite:
			delete pctx;
			return comm->thread_write();
		default:
			return 1;
		}
	}

	unsigned int CComm::thread_write()
	{
		DWORD nWritten;
		int nWrittenData;
		c_send_data_packet* psd = NULL;
		BOOL bRet;

		for (;;){
			if (!is_opened())
				return 0;

			psd = _send_data.get();
			//TODO

			nWrittenData = 0;
			while (nWrittenData < psd->cb){
				bRet = WriteFile(get_handle(), &psd->data[0] + nWrittenData, psd->cb - nWrittenData, &nWritten, NULL);
				if (!is_opened())
					return 0;

				if (bRet == FALSE){
					_notifier->msgerr("写串口设备时遇到错误");
					_data_counter.reset_unsend();
					_data_counter.call_updater();
					return 0;
				}
				if (nWritten == 0) continue;
				nWrittenData += nWritten;
				_data_counter.add_send(nWritten);
				_data_counter.sub_unsend(nWritten);
				_data_counter.call_updater();
			}

			_send_data.release(psd);
		}
		return 0;
	}

	unsigned int CComm::thread_read()
	{
		DWORD nRead, nTotalRead = 0, nBytesToRead;
		unsigned char* block_data = NULL;
		BOOL retval;

		block_data = (unsigned char*)malloc(COMMON_READ_BUFFER_SIZE);
		if (block_data == NULL){
			_notifier->msgbox(MB_ICONERROR, COMMON_NAME, "读线程结束!");
			return 1;
		}

		for (;;){
			COMSTAT sta;
			DWORD comerr;

			if (!is_opened())
				goto _exit;

			ClearCommError(get_handle(), &comerr, &sta);
			if (sta.cbInQue == 0){
				sta.cbInQue++;
			}

			nBytesToRead = sta.cbInQue;
			if (nBytesToRead >= COMMON_READ_BUFFER_SIZE){
				nBytesToRead = COMMON_READ_BUFFER_SIZE;
			}

			for (nTotalRead = 0; nTotalRead < nBytesToRead;){
				retval = ReadFile(get_handle(), &block_data[0] + nTotalRead, nBytesToRead - nTotalRead, &nRead, NULL);
				if (!is_opened())
					goto _exit;

				if (retval == FALSE){
					_data_counter.reset_unsend();
					_notifier->msgerr("读串口时遇到错误!\n"
						"是否在拔掉串口之前忘记了关闭串口先?\n\n"
						"错误原因");
					goto _exit;
				}
				if (nRead == 0) continue;
				nTotalRead += nRead;
				_data_counter.add_recv(nRead);
				_data_counter.call_updater();
			}//for::读nBytesRead数据

			//上一次的数据可能还在处理中, 所以要等待
			// 因为不在同一线程中, 需要同步
			wait_read_event();
			call_data_receivers(block_data, nBytesToRead);
		}
	_exit:
		if (block_data){
			free(block_data);
		}
		return 0;
	}

	void CComm::call_data_receivers(const unsigned char* ba, int cb)
	{
		_data_receiver_lock.lock();
		for (int i = 0; i < _data_receivers.size(); i++){
			_data_receivers[i]->receive(ba, cb);
		}
		_data_receiver_lock.unlock();
	}

	void CComm::remove_data_recerver(i_data_receiver* receiver)
	{
		_data_receiver_lock.lock();
		_data_receivers.remove(receiver);
		_data_receiver_lock.unlock();

	}

	void CComm::add_data_recerver(i_data_receiver* receiver)
	{
		_data_receiver_lock.lock();
		_data_receivers.add(receiver);
		_data_receiver_lock.unlock();
	}

	bool CComm::setting_comm(s_setting_comm* pssc)
	{
		SMART_ASSERT(is_opened()).Fatal();

		unsigned long ccsize = sizeof(_commconfig);
		if (!::GetCommConfig(get_handle(), &_commconfig, &ccsize)){
			_notifier->msgerr("取串口默认配置出错");
			return false;
		}
		if (!::GetCommState(get_handle(), &_commconfig.dcb)){
			_notifier->msgerr("取串口状态时出错");
			return false;
		}

		_commconfig.dcb.fBinary = TRUE;
		_commconfig.dcb.BaudRate = pssc->baud_rate;
		_commconfig.dcb.fParity = pssc->parity == NOPARITY ? FALSE : TRUE;
		_commconfig.dcb.Parity = pssc->parity;
		_commconfig.dcb.ByteSize = pssc->databit;
		_commconfig.dcb.StopBits = pssc->stopbit;

		if (!::SetCommConfig(get_handle(), &_commconfig, sizeof(_commconfig))){
			_notifier->msgerr("COM配置错误");
			return false;
		}
		if (!::SetCommMask(get_handle(), EV_RXCHAR)){
			_notifier->msgerr("SetCommMask()错误");
			return false;
		}
		if (!::SetCommTimeouts(get_handle(), &_timeouts)){
			_notifier->msgerr("设置串口超时错误");
			//TOOD: show config
			return false;
		}
		//TODO: size
		if (!::SetupComm(get_handle(), COMMON_READ_BUFFER_SIZE, COMMON_READ_BUFFER_SIZE)){
			_notifier->msgerr("SetupComm");
			return false;
		}

		PurgeComm(_hComPort, PURGE_TXCLEAR | PURGE_TXABORT);
		PurgeComm(_hComPort, PURGE_RXCLEAR | PURGE_RXABORT);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	c_data_packet_manager::c_data_packet_manager()
		: _hEvent(0)
	{
		list_init(&_list);
		_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		SMART_ASSERT(_hEvent != NULL).Fatal();

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
		SMART_ASSERT(size > 0)(size).Fatal();
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

		if (psdp->type == csdp_type::csdp_local){
			_lock.lock();
			psdp->used = false;
			_lock.unlock();
		}
		else{
			memory.free((void**)&psdp, "");
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
		// TODO
	}

}
