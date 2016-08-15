#include "stdafx.h"
#include "../res/resource.h"

#include "pinctrl.h"

static int axisx = -1, axisy = -1;

static int dtr[3] = {DTR_CONTROL_DISABLE, DTR_CONTROL_ENABLE, DTR_CONTROL_HANDSHAKE};
static int rts[3] = {RTS_CONTROL_DISABLE, RTS_CONTROL_ENABLE, RTS_CONTROL_HANDSHAKE};
static const char* sdtr[3] = {"(1)DTR_CONTROL_DISABLE", "(0)DTR_CONTROL_ENABLE", "(0)DTR_CONTROL_HANDSHAKE"};
static const char* srts[3] = {"(1)RTS_CONTROL_DISABLE", "(0)RTS_CONTROL_ENABLE", "(0)RTS_CONTROL_HANDSHAKE"};

static std::function<HANDLE()> _get_handle;
static HWND hDtr, hRts;

INT_PTR __stdcall DlgProc(HWND hwnd, UINT umsg, WPARAM wp, LPARAM lp) 
{
    switch(umsg)  {
    case WM_CLOSE:
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        axisx = rc.left;
        axisy = rc.top;
        EndDialog(hwnd, 0);
        return 0;
    }
    case WM_COMMAND:
    {
        switch(LOWORD(wp)) 
        {
        case IDC_PINCTRL_OK:
        {
            if(HIWORD(wp) != BN_CLICKED)
                return 0;
            if(!_get_handle()) {
                MessageBox(hwnd, "没有串口设备被打开!", "", MB_ICONINFORMATION);
                return 0;
            }

            COMMCONFIG _config;
            DWORD size = sizeof(_config);
            if(!GetCommConfig(_get_handle(), &_config, &size)) {
                MessageBox(hwnd, "获取串口配置时错误", nullptr, MB_ICONERROR);
                EndDialog(hwnd, 0);
                return 0;
            }
            _config.dcb.fDtrControl = dtr[ComboBox_GetCurSel(hDtr)];
            _config.dcb.fRtsControl = rts[ComboBox_GetCurSel(hRts)];
            if(!SetCommConfig(_get_handle(), &_config, sizeof(_config))) {
                MessageBox(hwnd, "设置DTR/RTS时错误!", nullptr, MB_ICONERROR);
                return 0;
            }
            break;
        }
        }
        return 0;
    }
    case WM_INITDIALOG:
    {
        if(!_get_handle()) {
            ::MessageBox(GetActiveWindow(), "请先打开一个串口设备!", "", MB_ICONEXCLAMATION);
            EndDialog(hwnd, 0);
            return 0;
        }

        COMMCONFIG _config;
        DWORD size = sizeof(_config);
        if(!GetCommConfig(_get_handle(), &_config, &size)) {
            MessageBox(GetActiveWindow(), "获取串口配置时错误", nullptr, MB_ICONERROR);
            EndDialog(hwnd, 0);
            return 0;
        }

        if(axisx == -1 && axisy == -1) {
            axisx = axisy = 300;
        }
        SetWindowPos(hwnd, 0, axisx, axisy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        ShowWindow(hwnd, SW_SHOW);

        hDtr = GetDlgItem(hwnd, IDC_CBO_PINCTRL_DTR);
        hRts = GetDlgItem(hwnd, IDC_CBO_PINCTRL_RTS);

        int i;
        for(i = 0; i < 3; i++) {
            ComboBox_AddString(hDtr, sdtr[i]);
            ComboBox_AddString(hRts, srts[i]);
        }
        ComboBox_SetCurSel(hDtr, _config.dcb.fDtrControl);
        ComboBox_SetCurSel(hRts, _config.dcb.fRtsControl);
    }
    }
    return 0;
}

void show_pinctrl(HWND owner, std::function<HANDLE()> get_com_handle) {
    _get_handle = get_com_handle;
    HWND hdlg = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DLG_PINCTRL), owner, DlgProc);
    hdlg = hdlg;
}

