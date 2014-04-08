#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#include <windows.h>

#define COMMON_NAME			"Com Monitor"
#define COMMON_VERSION		"1.15"

#ifdef _DEBUG 
	#define COMMON_NAME_AND_VERSION COMMON_NAME " " COMMON_VERSION " - Debug Mode"
#else
	#define COMMON_NAME_AND_VERSION COMMON_NAME " " COMMON_VERSION " "
#endif

struct about_s{
	void (*show)(void);
	void (*update)(void);
};

void init_about(void);


#ifndef __ABOUT_C__

extern struct about_s about;

#else

#undef __ABOUT_C__

void soft_update(void);
void show_about(void);
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
