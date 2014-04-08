#ifdef __cplusplus

#include <vector>
#include "struct/Thunk.h"
#include "struct/parse_cmd.h"

class ACmdItem
{
public:
	ACmdItem(HWND hWndParent,command_item* pci,int id);
	~ACmdItem();

public:
	struct ClassMsg
	{
		HWND hWnd;
		UINT uMsg;
		WPARAM wParam;
		LPARAM lParam;
		void* extra;
	};

public:
	HWND GetParent(void) const
	{
		return m_hParent;
	}
	HWND GethWnd(void) const
	{
		return m_hWnd;
	}
	void SethWnd(HWND hWnd)
	{
		m_hWnd = hWnd;
	}

	INT_PTR HandleMsg(UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR SetResult(LONG result,bool bHandled=true);
	INT_PTR __stdcall EditProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:
	void UpdatePci(void);

private:
	HWND hName;
	HWND hType;
	HWND hSize;
	HWND hCmd;
	HWND hSend;
	AThunk m_EditThunk;
	WNDPROC oldEditProc;

private:
	int m_id;
	command_item* m_pci;
	UINT m_uMsg;
	HWND m_hWnd;
	HWND m_hParent;
	const char* cmd_name;
	int			cmd_type;
	size_t		cmd_size;
};

class ASendCmd
{
public:
	ASendCmd(const char* fn);
	~ASendCmd();
	HWND GethWnd()
	{
		return m_hWnd;
	}

private:
	INT_PTR __stdcall DialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	INT_PTR SetResult(LONG result,bool bHandled=true);
	void ParseCmdFile(void);

private:
	HWND m_hWnd;
	UINT m_uMsg;
	AThunk m_Thunk;
	char m_fn[MAX_PATH];		//当前命令文件的路径
	std::vector<ACmdItem*> m_ACmdItems;

	command_item* pItem;
	size_t nItems;
};

#endif



EXTERN_C HWND newCmdWindow(const char* fn);
