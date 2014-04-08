//#include <vld.h>

#include "msg.h"
#include "utils.h"
#include "about.h"
#include "comm.h"
#include "deal.h"
#include "debug.h"
#include "struct/memory.h"
#include "load_driver/load_driver.h"
#pragma warning(disable:4100) //unreferenced formal parameter(s)

static char* __THIS_FILE__ = __FILE__;

#if 1
int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	MSG message;
	init_msg();
	init_utils();
	init_about();
	init_comm();
	init_deal();
	init_driver();
	init_memory();
#ifdef _DEBUG
	AllocConsole();
#endif
	debug_out(("程序已运行\n"));
	msg.run_app();

	while(GetMessage(&message,NULL,0,0)){
		if(!TranslateAccelerator(msg.hWndMain,msg.hAccel,&message)){
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	debug_out(("程序已结束\n"));
#ifdef _DEBUG
	Sleep(1000);
	FreeConsole();
#endif
	MessageBeep(MB_OK);
	return 0;
}

#endif
