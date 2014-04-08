#include <windows.h>
#include <cstdio>
#include <iostream>

extern "C" {
#include "debug.h"
#include "utils.h"		//center_window
#include "msg.h"		//handle.hWndMain
}
#include "draw.h"

#include <GdiPlus.h>
using namespace Gdiplus;
using namespace std;
#pragma comment(lib,"gdiplus")

ULONG_PTR gdiplusToken;

static char* __THIS_FILE__ = __FILE__;

#define DRAW_POINTS_PER_SCREEN			360				//一个屏幕显示的坐标点的个数
#define DRAW_LINE_WIDTH					1				//线段的宽度
#define DRAW_FRAME_WIDTH				3				//坐标轴的线宽
#define DRAW_OFFSET_LEFT				40				//Y轴到左边的间隔
#define DRAW_OFFSET_BOTTOM				40				//X轴到底边的间隔
#define DRAW_OFFSET_RIGHT				20				//Y轴到右边的间隔
#define DRAW_OFFSET_TOP					20				//X轴到上边的间隔
#define DRAW_POINTS_CLEARANCE			2				//两点之间的X轴(水平)的间隔
#define DRAW_HEIGHT_MULTIPLIER			1				//Y坐标放大倍数
#define DRAW_COLOR_LINE					RGB(255,0,0)	//线条的颜色
#define DRAW_COLOR_FRAME				RGB(255,255,255)//坐标轴颜色

static HWND hWndDrawWindow;
static int client_width,client_height;

//用于保存窗口坐标
static int axisx,axisy;



void draw_axis(Graphics* g)
{
	unsigned char testa[] = {
		12,52,47,54,245,12,98,69,46,30,19,139,238,189,65,3,5,120,34,255,0,43,67,111,167,34,25,78
	};
	
	Pen pen(Color(255,255,0,0));
	Pen gray(Color(150,192,192,192));

	g->DrawLine(&gray,DRAW_OFFSET_LEFT,DRAW_OFFSET_TOP,DRAW_OFFSET_LEFT,client_height-DRAW_OFFSET_BOTTOM);
	g->DrawLine(&gray,DRAW_OFFSET_LEFT,client_height-DRAW_OFFSET_BOTTOM,client_width-DRAW_OFFSET_RIGHT,client_height-DRAW_OFFSET_BOTTOM);

	int ax = 30;
	int ay = 300;
	for(int x=0; x<sizeof(testa)/sizeof(*testa)-1; x+=1){
		g->DrawLine(&pen,ax,ay-testa[x],ax+20,ay-testa[x+1]);
		ax += 20;
	}
	//Font font()
	//g->DrawString()
}




































LRESULT CALLBACK DrawWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		{
			return 0;
		}
	case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;
			hdc=BeginPaint(hWnd,&ps);
			Graphics g(hdc);
			Pen pen(Color(255,255,0,0));
			draw_axis(&g);
			EndPaint(hWnd,&ps);
			return 0;
		}
	case WM_LBUTTONDOWN:
		SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0);
		return 0;
	case WM_CREATE:
		{
			for(;;){//设置窗口长宽
				RECT rcW;RECT rcC;
				int borderheight;
				int borderwidth;
				GetWindowRect(hWnd,&rcW);
				GetClientRect(hWnd,&rcC);
				borderheight = (rcW.bottom-rcW.top)-(rcC.bottom-rcC.top);
				borderwidth = (rcW.right-rcW.left)-(rcC.right-rcC.left);
				client_height = DRAW_OFFSET_TOP+DRAW_OFFSET_BOTTOM+256*DRAW_HEIGHT_MULTIPLIER;	//上下间距+Y坐标倍乘
				client_width = DRAW_OFFSET_LEFT+DRAW_OFFSET_RIGHT+DRAW_POINTS_PER_SCREEN*DRAW_POINTS_CLEARANCE;
				MoveWindow(hWnd,axisx,axisy,client_width+borderwidth,client_height+borderheight,TRUE);

				GdiplusStartupInput gdiplusInput;
				GdiplusStartup(&gdiplusToken,&gdiplusInput,NULL);
				break;
			}
			return 0;
		}
	case WM_CLOSE:
		{
			RECT rc;
			GetWindowRect(hWnd,&rc);
			axisx=rc.left;
			axisy=rc.top;
			DestroyWindow(hWnd);
			hWndDrawWindow = NULL;
			GdiplusShutdown(gdiplusToken);
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int ShowDrawWindow(void)
{
	WNDCLASSEX wcx = {0};
	HINSTANCE hInst = NULL;
	if(hWndDrawWindow){
		if(IsIconic(hWndDrawWindow))
			ShowWindow(hWndDrawWindow,SW_RESTORE);
		SetWindowPos(hWndDrawWindow,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		return 1;
	}
	hInst = GetModuleHandle(NULL);
	wcx.cbSize = sizeof(wcx);
	wcx.style = CS_HREDRAW|CS_VREDRAW;
	wcx.lpfnWndProc = DrawWndProc;
	wcx.hInstance = hInst;
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
	wcx.lpszMenuName = NULL;
	wcx.lpszClassName = "DrawWindowClass";
	wcx.hIconSm = wcx.hIcon;
	RegisterClassEx(&wcx);
	hWndDrawWindow = CreateWindowEx(0, "DrawWindowClass", "Draw Test",
		WS_OVERLAPPEDWINDOW&~WS_MAXIMIZEBOX&~WS_SIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
		msg.hWndMain, NULL, hInst, NULL); 
	if(hWndDrawWindow == NULL) return 0;
	ShowWindow(hWndDrawWindow, SW_SHOWNORMAL);
	UpdateWindow(hWndDrawWindow);
	return 1;
}
