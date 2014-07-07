#define __DEAL_C__
#include "deal.h"
#include "common.h"
#include "msg.h"
#include "utils.h"
#include "comm.h"
#include "about.h"
#include "debug.h"
#include "draw.h"
#include "struct/memory.h"
#include "../res/resource.h"

#pragma comment(lib,"WinMM")	//timeSetEvent,timeKillEvent

struct deal_s deal;
static char* __THIS_FILE__ = __FILE__;

void init_deal(void)
{
	memset(&deal,0,sizeof(deal));
	deal.do_check_recv_buf     = do_check_recv_buf;
	deal.do_buf_recv           = do_buf_recv;
	deal.do_buf_send           = do_buf_send;
	deal.update_savebtn_status = update_savebtn_status;
	deal.update_status         = update_status;
	deal.thread_read           = thread_read;
	deal.thread_write          = thread_write;
	deal.cancel_auto_send      = cancel_auto_send;
	deal.check_auto_send       = check_auto_send;
	deal.do_send               = do_send;
	deal.send_char_data		   = send_char_data;
	deal.make_send_data        = make_send_data;
	deal.add_send_packet       = add_send_packet;
	deal.start_timer           = start_timer;
	deal.add_text			   = add_text;
	deal.add_text_critical     = add_text_critical;
	deal.last_show             = 1;

	InitializeCriticalSection(&deal.g_add_text_cs);
}

//更新 保存到文件 按钮的状态
//应lin0119的要求, 同时更新"复制接收数据"的状态
void update_savebtn_status(void)
{
	HWND hSave = GetDlgItem(msg.hWndMain, IDC_BTN_SAVEFILE);
	HWND hCopy = GetDlgItem(msg.hWndMain, IDC_BTN_COPY_RECV);
	//串口没打开或不显示数据
	//PS:我自己现在都分不清楚到底是IHV或是NULL代表无效, 算了, 认了, 不过以后肯定知道该怎么搞了
	BOOL bEnable = msg.hComPort==INVALID_HANDLE_VALUE || !msg.hComPort || !comm.fShowDataReceived;
	EnableWindow(hSave,bEnable);
	EnableWindow(hCopy,bEnable);
}

//NULL:更新计数状态
void update_status(char* str)
{
#define STATUS_LENGTH	6
	static char status[128] = {" 状态:"};
	static HWND hStatus;
	//TODO:
	if(!hStatus)
		hStatus = GetDlgItem(msg.hWndMain, IDC_STATIC_STATUS);
	if(str == NULL)//更新计数状态
		sprintf(status+STATUS_LENGTH, "接收计数:%u,发送计数:%u,等待发送:%u", comm.cchReceived, comm.cchSent,comm.cchNotSend);
	else//输出状态语句
		_snprintf(status+STATUS_LENGTH, sizeof(status)-STATUS_LENGTH, "%s", str);
	SetWindowText(hStatus, status);
#undef STATUS_LENGTH
}
/*
函  数:do_buf_recv
功  能:缓冲停止显示后的数据
参  数:
	chs:字符指针,跟动作相关
	cb:字节数
	action:动作, 有如下:
		0 - 添加缓冲内存, 无返回值
		1 - 取得缓冲区内容, 返回值为unsigned char*
		2 - 取得缓冲区的长度, 返回int
		3 - 释放缓冲区内存
返回值:
	int - 参看action动作部分
*/

// #pragma pack(push,1)
// struct Frame{
// 	unsigned char h1;
// 	unsigned char currentFrame;
// 	unsigned char data[240];
// 	unsigned char h2[2];
// };
// #pragma pack(pop)

int do_buf_recv(unsigned char* chs, int cb, int action)
{
	static unsigned char* str = NULL;
	static unsigned char* pstr = NULL;
	//unsigned int length = 0;
	if(str == NULL && action==0){
		//TODO:
		str = (unsigned char*)GET_MEM(COMMON_INTERNAL_RECV_BUF_SIZE);
		if(str==NULL) return 0;
		pstr = str;
	}
	switch(action)
	{
	case 0://添加缓冲字符
		if(pstr-str + cb > COMMON_INTERNAL_RECV_BUF_SIZE){
			int ret;
			//TODO:询问是否更新到显示后再继续,不必删除
			ret = utils.msgbox(msg.hWndMain,MB_ICONERROR|MB_YESNO, "警告",
				"停止显示后, 显示的数据被保存到了程序内部的缓冲区, 但:\n"
				"内部缓冲区默认的1M空间已满, 数据可能已部分丢失!\n\n"
				"是否要清空内部缓冲区?"
			);
			if(ret == IDYES){
				memory.free_mem((void**)&str,NULL);
			}
			return 0;
		}
		memcpy(pstr,chs,cb);
		pstr += cb;
		//length = sprintf(pstr, chs);
		//pstr += length;
		break;
	case 1://取得缓冲区内容
		return (int)str;
	case 2://取得缓冲区长度
		return (int)(pstr-(unsigned int)str/*+1*/);
	case 3://释放缓冲区
		if(str) memory.free_mem((void**)&str,"<do_buf_recv>");
		return 0;
	}
	return 0;
}

/**************************************************
函  数:do_check_recv_buf@-
功  能:在由停止显示恢复到继续显示时检测缓冲区是否有保存的数据
参  数:(none)
返回值:(none)
说  明:(none)
**************************************************/
void do_check_recv_buf(void)
{
	unsigned char* saved = (unsigned char*)do_buf_recv(NULL, 0, 1);
	if(saved != NULL){//内部缓冲区有数据
		int ret;
		ret = utils.msgbox(msg.hWndMain,MB_ICONINFORMATION|MB_YESNO, "提示",
			"嗯~\n\n"
			"在暂停显示数据后, 未被显示的数据被保存到了内部缓冲区中,\n\n"
			"需要更新被保存的数据到接收区吗?\n\n"
			"如果您选择了否, 这部分数据将不会再被保存!\n\n"
			"缓冲区目前已有 %d 字节的数据~",
			do_buf_recv(NULL,0,2)); //因为已经设置了事件, 所以在这里, 缓冲区的数据量不会改变
		if(ret == IDYES){//希望显示保存起来了的数据
			/*int len1 = Edit_GetTextLength(msg.hEditRecv);
			int len2 = do_buf_recv(NULL,0, 2);
			if(len1+len2 > COMMON_RECV_BUF_SIZE){//但缓冲区装不下了
				int ret;
				ret = utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION|MB_YESNO, COMMON_NAME,
					"喔~~~\n\n"
					"缓冲区的长度已达到最大限制, 数据可能不会被完整地显示到接收区中, 要显示截断后的内容吗?\n\n"
					"如果选择了否, 内部保存的缓冲区数据会被清除的哦~");
				if(ret == IDNO){//取消显示并清空
					do_buf_recv(NULL,0, 3);
					return;
				}else{//选择了截断显示数据
					
				}
			}
			Edit_SetSel(msg.hEditRecv, len1, len1);
			Edit_ReplaceSel(msg.hEditRecv, saved);*/
			add_text(saved,do_buf_recv(NULL,0,2));
			do_buf_recv(NULL, 0, 3);
		}else{//不需要显示保存的数据,则清空
			do_buf_recv(NULL, 0, 3);
		}
	}
}

/***********************************************************************
名称:do_buf_send
描述:缓冲未被发送的数据
参数:action-动作,pv-动作参数
返回:
说明:动作参数仅当action为SEND_DATA_ACTION_RETURN时有效,为SEND_DATA指针
***********************************************************************/
#define SEND_DATA_SIZE 101
int do_buf_send(int action,void* pv)
{
	static SEND_DATA* send_data[SEND_DATA_SIZE];
	//static 
	int retval;
	if(action==SEND_DATA_ACTION_GET || action==SEND_DATA_ACTION_RETURN || action==SEND_DATA_ACTION_RESET){
		EnterCriticalSection(&deal.critical_section);
	}
	switch(action)
	{
	case SEND_DATA_ACTION_INIT://初始化
		{
			int it;
			int len = sizeof(SEND_DATA)*SEND_DATA_SIZE;
			void* pv=GET_MEM(len);
			if(!pv){
				utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"初始化发送缓冲区失败,请重新运行程序!");
				return 0;
			}
			for(it=0; it<SEND_DATA_SIZE; it++){
				send_data[it] = (SEND_DATA*)((unsigned char*)pv+it*sizeof(SEND_DATA));
				send_data[it]->flag = SEND_DATA_TYPE_NOTUSED;
			}
			InitializeCriticalSection(&deal.critical_section);
			return 1;
		}
	case SEND_DATA_ACTION_GET://取得缓冲区
		{
			int i;
			for(i=0;i<SEND_DATA_SIZE;i++){
				if(send_data[i]->flag == SEND_DATA_TYPE_NOTUSED){
					send_data[i]->flag = SEND_DATA_TYPE_USED;
					retval = (int)send_data[i];
					goto _exit_dbs;
				}
			}
			retval = 0;
			goto _exit_dbs;
		}
	case SEND_DATA_ACTION_RETURN://归还缓冲区
		{
			int i;
			for(i=0;i<SEND_DATA_SIZE;i++){
				if((SEND_DATA*)pv == send_data[i]){
					send_data[i]->flag = SEND_DATA_TYPE_NOTUSED;
					retval = 0;
					break;
				}
			}
			goto _exit_dbs;
		}
	case SEND_DATA_ACTION_FREE://释放所有缓冲区
		memory.free_mem((void**)&send_data[0],NULL);
		DeleteCriticalSection(&deal.critical_section);
		return 1;
	case SEND_DATA_ACTION_RESET://复位所有
		{
			int i;
			for(i=0;i<SEND_DATA_SIZE;i++){
				send_data[i]->flag = SEND_DATA_TYPE_NOTUSED;
			}
			retval = 0;
			goto _exit_dbs;
		}
	}
_exit_dbs:
	LeaveCriticalSection(&deal.critical_section);
	return retval;
}

/**************************************************
函  数:add_text@8
功  能:添加ba指向的数据到显示区(16进制和字符模式)
参  数:ba - 16进制数据,cb - 字节数
返回值:(none)
说  明:
	2013-03-10:作了大量修改, 貌似现在不再丢包?找到这个
BUG可花了我不少时间!!!
	2014-07-06:增加控制字符处理
**************************************************/

static void add_text_helper(char* str)
{
	int len;
	unsigned int cntb = utils.eliminate_control_char(str);
	char* p = str;
	do{
		if(*p != '\b'){ // --- 追加
			len = Edit_GetTextLength(msg.hEditRecv2);
			Edit_SetSel(msg.hEditRecv2,len,len);
			Edit_ReplaceSel(msg.hEditRecv2,p);
		}
		else{ // --- 向前删除
			len = Edit_GetTextLength(msg.hEditRecv2);
			if(len){
				Edit_SetSel(msg.hEditRecv2, len-1,len);
				Edit_ReplaceSel(msg.hEditRecv2,"");
			}
			p++;
		}
	}while(cntb--);
}

void add_text_critical(unsigned char* ba, int cb)
{
	//2012-03-19:增加到10KB空间
	static char inner_str[10240];
	if(cb==0) return;
	//	draw_line(ba,cb);
	if(comm.fShowDataReceived){
		if(comm.data_fmt_recv){//16进制
			char* str=NULL;
			DWORD len,cur_pos;
			len = comm.data_count;//Edit_GetTextLength(msg.hEditRecv);
			cur_pos = len % (COMMON_LINE_CCH_RECV*3+2);
			cur_pos = cur_pos/3;
			str = utils.hex2str(ba,&cb,COMMON_LINE_CCH_RECV,cur_pos,inner_str,__ARRAY_SIZE(inner_str));
			__try{
				// 不应该在非主线程里面操作UI, 可能会出错
				Edit_SetSel(msg.hEditRecv, len, len);
				Edit_ReplaceSel(msg.hEditRecv, str);
				if(str!=inner_str) memory.free_mem((void**)&str,NULL);
			}
			__except(EXCEPTION_EXECUTE_HANDLER){
				utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"add_text:Access Violation!");
			}
			InterlockedExchangeAdd((long volatile*)&comm.data_count,cb);
		}else{//字符
			char* str=NULL;
			if(comm.fDisableChinese){//不允许显示中文的话,把所有>0x7F的字符改成'?',同样也处理特殊字符
				int it;
				unsigned char uch;
				for(it=0; it<cb; it++){
					uch = ba[it];

					if(uch>0 && uch<32 && (uch!='\n' && (uch=='\b' && !comm.fEnableControlChar)) || uch>0x7F){ //看得懂不? ^_^
						ba[it] = (unsigned char)'?';
					}

				}
			}

			str=utils.hex2chs(ba,cb,inner_str,__ARRAY_SIZE(inner_str));
			add_text_helper(str);
			if(str!=inner_str) memory.free_mem((void**)&str,NULL);
		}
	}else{
		do_buf_recv(ba,cb,0);
	}
}

void add_text(unsigned char* ba, int cb)
{
	EnterCriticalSection(&deal.g_add_text_cs);
	add_text_critical(ba, cb);
	LeaveCriticalSection(&deal.g_add_text_cs);
}

/***********************************************************************
名称:thread_read
描述:用来读取串口数据的工作线程
参数:pv - 未使用
返回:未使用
说明:本来改成异步IO的,结果不知道怎么回事,WaitForMultipleObjects总是失败,写得好好
的一段代码,彻底失败了~我去,以后换用WaitForCommEvent再来,擦!
***********************************************************************/
unsigned int __stdcall thread_read(void* pv)
{
	DWORD nRead,nTotalRead=0,nBytesToRead;
	unsigned char* block_data=NULL;
	BOOL retval;

	block_data = (unsigned char*)GET_MEM(COMMON_READ_BUFFER_SIZE);
	if(block_data == NULL){
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"读线程结束!");
		return 1;
	}

	for(;;){
		COMSTAT sta;
		DWORD comerr;
		int flag=0;//中文检测时用到

		if(comm.fCommOpened==FALSE || msg.hComPort==INVALID_HANDLE_VALUE){
			debug_out(("因为设备句柄无效,读线程退出!\n"));
			//utils.msgbox(msg.hWndMain,MB_ICONERROR,"设备句柄无效,读线程终止!");
			//return 0;
			goto _exit;
		}

		ClearCommError(msg.hComPort,&comerr,&sta);
		if(sta.cbInQue == 0){
			sta.cbInQue++;
		}
			
		nBytesToRead = sta.cbInQue;
		//保留最后一个字节用来容纳期望的中文字符
		if(nBytesToRead>=COMMON_READ_BUFFER_SIZE){
			nBytesToRead = COMMON_READ_BUFFER_SIZE-1;
		}

		nTotalRead = 0;
		for(;;){
			for(;nTotalRead<nBytesToRead;){
				//debug_out(("进入ReadFile\n"));
				retval = ReadFile(msg.hComPort, &block_data[0]+nTotalRead, nBytesToRead-nTotalRead, &nRead, NULL);
				if(comm.fCommOpened == FALSE || msg.hComPort==INVALID_HANDLE_VALUE){
					debug_out(("ReadFile因为comm.fCommOpened==FALSE而退出!\n"));
					goto _exit;
				}
				if(retval == FALSE){
					InterlockedExchange((long volatile*)&comm.cchNotSend,0);
					//comm.close();
					//comm.update((int*)-1);
					if(comm.fAutoSend){
						deal.cancel_auto_send(0);
					}
					utils.msgerr(msg.hWndMain,"读串口时遇到错误!\n"
						"是否在拔掉串口之前忘记了关闭串口先?\n\n"
						"错误原因");
					SetTimer(msg.hWndMain,TIMER_ID_THREAD,100,NULL);
					goto _exit;
				}
				
				if(nRead == 0) continue;
				nTotalRead += nRead;
				InterlockedExchangeAdd((long volatile*)&comm.cchReceived, nRead);
				update_status(NULL);
			}//for::读nBytesRead数据
			//debug_out(("中文检测...\n"));
			//读完了要求读的字节数,再做中文完整性检测
			if(!flag && !comm.fDisableChinese && utils.check_chs(&block_data[0],nTotalRead)){
				int it;
				debug_out(("因为中文,两次等待ReadFile\n"));
				for(it=0;it<20;it++){
					ClearCommError(msg.hComPort,&comerr,&sta);
					if(sta.cbInQue){
						nBytesToRead += 1;
						flag = 1;
						goto _continue_read;
					}else{
						Sleep(50);
					}
				}
				block_data[nBytesToRead-1]='.';
				//nBytesToRead--;
				break;
			}

			// 如果一个'\r\n'被分两次读取, 那么她可能被处理成两个回车换行, 所以应该一次性把'\r','\n'读完(指定时间内)
			if(block_data[nBytesToRead-1]=='\r' || block_data[nBytesToRead-1]=='\n'){
				int it;
				for(it=0; it<40; it++){
					ClearCommError(msg.hComPort, &comerr, &sta);
					if(sta.cbInQue){
						nBytesToRead += 1;
						goto _continue_read;
					}
					else{
						Sleep(50);
					}
				}
				break;
			}

			break;
_continue_read:
			;
		}
		WaitForSingleObject(deal.hEventContinueToRead,INFINITE);
		add_text(&block_data[0],nBytesToRead);
	}
_exit:
	if(block_data){
		memory.free_mem((void**)&block_data,"读线程");
	}
	debug_out(("读线程自然退出!\n"));
	UNREFERENCED_PARAMETER(pv);
	return 0;
}

/***********************************************************************
名称:thread_write
描述:写串口设备线程
参数:pv-未使用
返回:未使用
说明:2013-04-13:改为异步写方式
2013-04-23:异步模式失败了,驱动不支持! 再改回同步模式!, 我擦, 要不要这样~~~
***********************************************************************/
unsigned int __stdcall thread_write(void* pv)
{
	DWORD nWritten,nRead,nWrittenData;
	SEND_DATA* psd = NULL;
	//OVERLAPPED overlap={0};
	//HANDLE hWriteEvent = CreateEvent(NULL,TRUE, FALSE,NULL);
	BOOL bRet;

	for(;;){
		if(comm.fCommOpened==FALSE || msg.hComPort==INVALID_HANDLE_VALUE){
			return 0;
		}

		bRet = ReadFile(deal.thread.hPipeRead,(void*)&psd,4,&nRead,NULL);
		if(bRet == FALSE){
			if(!comm.fCommOpened||!deal.thread.hPipeRead)
				return 0;
			utils.msgerr(msg.hWndMain,"读取管道时错误:");
		}
		if(nRead!=4)
			continue;
		//约定指针值为0x00000001时为退出(非分配内存)
		if((unsigned long)psd == 0x00000001){
			debug_out(("因为收到了数据为1的指针,写线程正在退出!\n"));
			return 0;
		}

		nWrittenData = 0;
		while(nWrittenData < psd->data_size){
			bRet = WriteFile(msg.hComPort, &psd->data[0]+nWrittenData,psd->data_size-nWrittenData, &nWritten, NULL);
			if(comm.fCommOpened==FALSE || msg.hComPort==INVALID_HANDLE_VALUE){
				debug_out(("因为comm.fCommOpened==FALSE或msg.hComPort,写线程退出!\n"));
				return 0;
			}
			if(bRet == FALSE){
				if(comm.fAutoSend){
					deal.cancel_auto_send(0);
				}
				utils.msgerr(msg.hWndMain,"写串口设备时遇到错误");
				InterlockedExchange((long volatile*)&comm.cchNotSend,0);
				update_status(NULL);
				//comm.close();
				//comm.update((int*)-1);
				SetTimer(msg.hWndMain,TIMER_ID_THREAD,100,NULL);
				return 0;
			}
			if(nWritten==0) continue;
			//debug_out(("in WriteFile,nWritten==%u\n",nWritten));
			nWrittenData += nWritten;
			InterlockedExchangeAdd((volatile long*)&comm.cchSent,nWritten);				//发送计数   - 增加
			InterlockedExchangeAdd((volatile long*)&comm.cchNotSend,-(LONG)nWritten);	//未发送计数 - 减少
			update_status(NULL);
		}
		if(psd->flag==SEND_DATA_TYPE_USED || psd->flag==SEND_DATA_TYPE_AUTO_USED)
			do_buf_send(SEND_DATA_ACTION_RETURN,(void*)psd);
		else if(psd->flag==SEND_DATA_TYPE_MUSTFREE || psd->flag==SEND_DATA_TYPE_AUTO_MUSTFREE)
			memory.free_mem((void**)&psd,"被写完的数据");
			
	}
	UNREFERENCED_PARAMETER(pv);
	debug_out(("写线程自然退出!\n"));
	return 0;
}


/**************************************************
函  数:cancel_auto_send@4
功  能:取消自动发送操作
参  数:reason-取消理由:0-check,1-关闭串口
返回值:(none)
说  明:无论串口是否打开
	2013-03-04更新:串口关闭不再自动取消自动发送(打钩)
**************************************************/
void cancel_auto_send(int reason)
{
	//if(!comm.fAutoSend&&msg.hComPort!=INVALID_HANDLE_VALUE) return;
	EnableWindow(GetDlgItem(msg.hWndMain,IDC_BTN_SEND),TRUE);
	EnableWindow(GetDlgItem(msg.hWndMain,IDC_EDIT_DELAY),TRUE);

	if(reason==1){

	}else if(reason == 0){
		CheckDlgButton(msg.hWndMain,IDC_CHK_AUTO_SEND,FALSE);
	}
	if(comm.fAutoSend){
		if(deal.timer_id){
			timeKillEvent(deal.timer_id);
			deal.timer_id = 0;
		}
		comm.fAutoSend=0;
	}
}

/**************************************************
函  数:check_auto_send@-
功  能:使能自动发送选项
参  数:(none)
返回值:(none)
说  明:无论串口是否打开
**************************************************/
static void __stdcall AutoSendTimerProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	if(!comm.fAutoSend || msg.hComPort==INVALID_HANDLE_VALUE){
		deal.cancel_auto_send(0);
		debug_out(("自动指针已释放\n"));
		memory.free_mem((void**)deal.autoptr,"AutoSendTimerProc");
		return;
	}

	do_send(2);
	
	UNREFERENCED_PARAMETER(uID);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(dwUser);
	UNREFERENCED_PARAMETER(dw1);
	UNREFERENCED_PARAMETER(dw2);
	return;
}
void check_auto_send(void)
{
	int flag;
	int elapse;
	BOOL fTranslated;


	flag = IsDlgButtonChecked(msg.hWndMain, IDC_CHK_AUTO_SEND);
	if(!flag){
		deal.cancel_auto_send(0);
		return;
	}else{
		int len = GetWindowTextLength(GetDlgItem(msg.hWndMain,IDC_EDIT_SEND));
		if(len == 0){
			utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,COMMON_NAME,
				"请输入待发送的数据后再选择自动发送!");
			CheckDlgButton(msg.hWndMain,IDC_CHK_AUTO_SEND,FALSE);
			return;
		}
	}
	elapse = GetDlgItemInt(msg.hWndMain,IDC_EDIT_DELAY,&fTranslated,FALSE);
	if(!fTranslated || (elapse>60000||elapse<10)){
		utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,COMMON_NAME,
			"自动发送时间设置不正确, 自动发送被否决!\n时间范围为10ms~60000ms");
		CheckDlgButton(msg.hWndMain,IDC_CHK_AUTO_SEND,FALSE);
		return;
	}
	
	EnableWindow(GetDlgItem(msg.hWndMain, IDC_EDIT_DELAY),FALSE);
	EnableWindow(GetDlgItem(msg.hWndMain, IDC_BTN_SEND),FALSE);

	if(msg.hComPort!=INVALID_HANDLE_VALUE){
		deal.autoptr = do_send(1);
		if(deal.autoptr==NULL){
			deal.cancel_auto_send(0);
			return;
		}
		deal.timer_id=timeSetEvent(elapse,0,AutoSendTimerProc,0,TIME_PERIODIC|TIME_CALLBACK_FUNCTION);
		comm.fAutoSend = 1;
	}
}

/**************************************************
函  数:make_send_data
功  能:从内存数据构建SEND_DATA数据包
参  数:	fmt,说明data数据的格式:
			0 - 16进制或字符,data指向普通内存
			1 - data指向SEND_DATA*
		data:见上
		size:数据大小
返  回:SEND_DATA* 或 NULL(失败)
说  明:
**************************************************/
SEND_DATA* make_send_data(int fmt,void* data,size_t size)
{
	SEND_DATA* psd = NULL;
	int is_buffer_enough = 0;

	//2013年11月2日 16:33:13 放这里明显不合适
	if(msg.hComPort==NULL||msg.hComPort==INVALID_HANDLE_VALUE){
		return NULL;
	}

	if(fmt == 0){
		is_buffer_enough = size<=sizeof(((SEND_DATA*)NULL)->data);
		if(is_buffer_enough){
			psd = (SEND_DATA*)deal.do_buf_send(SEND_DATA_ACTION_GET,NULL);
			if(psd) psd->cb = sizeof(SEND_DATA)-sizeof(((SEND_DATA*)NULL)->data)+size;
		}else{
			int total = sizeof(SEND_DATA)+size-sizeof(((SEND_DATA*)NULL)->data);
			psd = (SEND_DATA*)GET_MEM(total);
			if(psd) psd->cb = total;
		}
	}else if(fmt == 1){ //data为SEND_DATA*,直接复制整个psd
		is_buffer_enough = size<=sizeof(SEND_DATA);
		if(is_buffer_enough){
			psd = (SEND_DATA*)deal.do_buf_send(SEND_DATA_ACTION_GET,NULL);
			if(psd) psd->cb = sizeof(SEND_DATA)-sizeof(((SEND_DATA*)NULL)->data)+size;
		}else{
			psd = (SEND_DATA*)GET_MEM(size);
			if(psd) psd->cb = size;
		}
	}
	if(!psd){
		deal.cancel_auto_send(0);
		utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,"请等等...","发送速度过快,内部发送缓冲区没有空闲!\n\n"
			"如果已开启自动发送,则自动发送被取消!");
		return NULL;
	}

	if(fmt == 0){
		memcpy(psd->data,data,size);
		psd->data_size = size;
		//psd->flag = 需要在其它函数设置
		psd->flag = is_buffer_enough?SEND_DATA_TYPE_USED:SEND_DATA_TYPE_MUSTFREE;
	}else if(fmt==1){
		memcpy(psd,deal.autoptr,((SEND_DATA*)deal.autoptr)->cb);
		psd->flag = is_buffer_enough?SEND_DATA_TYPE_AUTO_USED:SEND_DATA_TYPE_AUTO_MUSTFREE;
	}
	
	return psd;
}
//可以和save_to_file整合到一起
int get_edit_data(int fmt,void** ppv,size_t* size)
{
	HWND hSend;			//待发送内容的EditBox
	char* buff = NULL;	//保存待发送内容
	size_t len;			//buff len
	unsigned char* bytearray=NULL;

	hSend = GetDlgItem(msg.hWndMain,IDC_EDIT_SEND);
	len = GetWindowTextLength(hSend);
	if(len == 0){
		return 0;
	}
	buff = (char*)GET_MEM(len+1);
	if(buff==NULL) return 0;
	GetWindowText(hSend,buff,len+1);

	if(fmt){		//16进制方式发送
		int ret;
		int length;
		bytearray = NULL;
		ret = utils.str2hex(buff,&bytearray,0);
		if(!(ret&0x80000000)){
			if(comm.fAutoSend){
				deal.cancel_auto_send(0);
			}
			length = ret & 0x7FFFFFFF;
			utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION, NULL, "发送区的数据解析错误, 请检查!\n\n是不是选错了发送数据的格式\?\n\n"
				"在第 %d 个字符附近出现语法解析错误!",length);
			memory.free_mem((void**)&buff,NULL);
			return 0;
		}
		//解析正确来到这里
		length = ret & 0x7FFFFFFF;
		len = length;
	}else{//字符方式
		len = utils.wstr2lstr(buff);//返回包括'\0'
		--len;
		//如果启用了以下2种 数据处理方式, 则原len变得无效
		if(comm.data_fmt_ignore_return){
			len = utils.remove_string_return(buff);
		}
		if(comm.data_fmt_use_escape_char){
			unsigned int ret = utils.parse_string_escape_char(buff);
			if(ret & 0x80000000){
				len = ret & 0x7FFFFFFF;
			}else{
				if(comm.fAutoSend){
					deal.cancel_auto_send(0);
				}
				len = ret & 0x7FFFFFFF;
				utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION, NULL, "解析转义字符串时遇到错误!\n\n"
					"在第 %d 个字符附近出现语法解析错误!",len);
				memory.free_mem((void**)&buff,NULL);
				return 0;
			}
		}
		
	}
	//len为16进制/字符数据最终实际长度
	if(fmt){
		*ppv = bytearray;
		memory.free_mem((void**)&buff,"");
	}else{
		*ppv = buff;
	}
	*size = len;
	
	return 1;
}


void add_send_packet(SEND_DATA* psd)
{
	DWORD nWritten=0;
	if(WriteFile(deal.thread.hPipeWrite,&psd,4,&nWritten,NULL) && nWritten==sizeof(psd)){
		InterlockedExchangeAdd((volatile long*)&comm.cchNotSend,psd->data_size);
	}else{
		utils.msgerr(NULL,"添加发送数据包时出错!");
	}
	update_status(NULL);
}

/**************************************************
函  数:do_send@4
功  能:发送到串口 && 自动发送相关
参  数:action-0:手动发送,1-由自动发送调用do_send函数(第1次,用于取得数据),2-自动发送的后续调用
返回值:数据指针
说  明:
	2013-05-11:
		重复发送时的操作改到了这里
		重复发送时先do_send(1)得到数据
		然后do_send(2)取得数据并复制到新的缓冲区并发送
	2013-10-15:
		由于一时疏忽,改动太多,测试过少,忘记加检测是否打开串口了, 现已纠正错误
**************************************************/
void* do_send(int action)
{
	void* pv = NULL;
	size_t size = 0;
	SEND_DATA* psd = NULL;

	if(msg.hComPort==NULL || msg.hComPort==INVALID_HANDLE_VALUE){
		utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,NULL,"请先打开串口设备~");
		return NULL;
	}

	if(action==0 || action==1){
		if(get_edit_data(comm.data_fmt_send,&pv,&size)){

			psd = make_send_data(0,pv,size);
			memory.free_mem((void**)&pv,"do_send_0");
			if(psd == NULL) return NULL;
			if(action == 0){
				add_send_packet(psd);
			}else if(action == 1){
				return psd;
			}
		}
		return NULL;
	}else if(action == 2){
		psd = make_send_data(1,deal.autoptr,((SEND_DATA*)deal.autoptr)->cb);
		if(psd) add_send_packet(psd);
		return NULL;
	}
	return NULL;
}

/**************************************************
函  数:send_char_data
功  能:发送一个char数据(来自接收数据框)
参  数:ch - 字符
返  回:1-成功,0-失败,-1-由于某些原因取消(比较串口并未打开)(成功)
说  明:
**************************************************/
int send_char_data(char ch)
{
	SEND_DATA* psd = NULL;
	if(msg.hComPort==NULL || msg.hComPort==INVALID_HANDLE_VALUE){
		return -1;
	}
	psd = make_send_data(0, &ch, 1);
	if(!psd) return 0;
	add_send_packet(psd);
	return 1;
}

/**************************************************
函  数:start_timer@4
功  能:开启计时器
参  数:start:!0-开启,0-关闭
返回值:(none)
说  明:
**************************************************/
static void __stdcall TimeProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	unsigned char *second,*minute,*hour;
	char str[9];
	second = (unsigned char *)((unsigned long)&deal.conuter + 0);
	minute = (unsigned char *)((unsigned long)&deal.conuter + 1);
	hour   = (unsigned char *)((unsigned long)&deal.conuter + 2);
	if(++*second == 60){
		*second = 0;
		if(++*minute == 60){
			*minute = 0;
			if(++*hour == 24){
				*hour = 0;
			}
		}
	}
	sprintf(&str[0],"%02d:%02d:%02d",*hour,*minute,*second);
	SetDlgItemText(msg.hWndMain,IDC_STATIC_TIMER,str);
	UNREFERENCED_PARAMETER(uID);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(dwUser);
	UNREFERENCED_PARAMETER(dw1);
	UNREFERENCED_PARAMETER(dw2);
}

void start_timer(int start)
{
	static UINT timer_id;
	if(start){
		InterlockedExchange((volatile long*)&deal.conuter,0);
		SetDlgItemText(msg.hWndMain,IDC_STATIC_TIMER,"00:00:00");
		timer_id=timeSetEvent(1000,0,TimeProc,0,TIME_PERIODIC|TIME_CALLBACK_FUNCTION);
		if(timer_id == 0){
			//...
		}
	}else{
		if(timer_id){
			timeKillEvent(timer_id);
			timer_id = 0;
		}
	}
}
