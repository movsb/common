//ts=sts=sw=4
//女孩不哭 2013-09-11
// http://www.cnblogs.com/nbsofer/p/thunk_in_cpp.html
#include <Windows.h>
#include "Thunk.h"

AThunk::AThunk()
{
	m_pthis = (AThunk*)VirtualAlloc(NULL,sizeof(*this),MEM_COMMIT,PAGE_EXECUTE_READWRITE);
}

AThunk::~AThunk()
{
	if(m_pthis){
		VirtualFree(m_pthis,0,MEM_RELEASE);
	}
}

void* AThunk::fnStdcall(void* pThis,void* mfn)
{
	/****************************************************************************************
	machine code					assembly code						comment
	------------------------------------------------------------------------------------------
	FF 34 24						push	dword ptr[esp]				;再次压入返回地址
	C7 44 24 04 ?? ?? ?? ??			mov		dword ptr[esp+4],this		;传入this指针
	E9 ?? ?? ?? ??					jmp		(relative target)			;转到成员函数
	****************************************************************************************/

	m_pthis->m_stdcall.push[0] = 0xFF;
	m_pthis->m_stdcall.push[1] = 0x34;
	m_pthis->m_stdcall.push[2] = 0x24;

	m_pthis->m_stdcall.mov = 0x042444C7;
	m_pthis->m_stdcall.pthis = (byte4)pThis;

	m_pthis->m_stdcall.jmp = 0xE9;
	m_pthis->m_stdcall.addr = (byte4)mfn-((byte4)&m_pthis->m_stdcall.jmp+5);

	FlushInstructionCache(GetCurrentProcess(),&m_pthis->m_stdcall,sizeof(m_pthis->m_stdcall));

	return &m_pthis->m_stdcall;
}

void* AThunk::fnCdeclcall(void* pThis,void* mfn)
{
	/****************************************************************************************
	machine code					assembly code						comment
	------------------------------------------------------------------------------------------
	3E 8F 05 ?? ?? ?? ??			pop		dword ptr ds:[?? ?? ?? ??]	;弹出并保存返回地址
	68 ?? ?? ?? ??					push	this						;压入this指针
	68 ?? ?? ?? ??					push	my_ret						;压入我的返回地址
	9E ?? ?? ?? ??					jmp		(relative target)			;跳转到成员函数
	83 C4 04						add		esp,4						;清除this栈
	3E FF 25 ?? ?? ?? ??			jmp		dword ptr ds:[?? ?? ?? ??]	;转到原返回地址
	****************************************************************************************/
	m_pthis->m_cdecl.pop_ret[0] = 0x3E;
	m_pthis->m_cdecl.pop_ret[1] = 0x8F;
	m_pthis->m_cdecl.pop_ret[2] = 0x05;
	*(byte4*)&m_pthis->m_cdecl.pop_ret[3] = (byte4)&m_pthis->m_cdecl.ret_addr;

	
	m_pthis->m_cdecl.push_this[0] = 0x68;
	*(byte4*)&m_pthis->m_cdecl.push_this[1] = (byte4)pThis;
	
	m_pthis->m_cdecl.push_my_ret[0] = 0x68;     
	*(byte4*)&m_pthis->m_cdecl.push_my_ret[1] = (byte4)&m_pthis->m_cdecl.add_esp[0];
	
	m_pthis->m_cdecl.jmp_mfn[0] = 0xE9;
	*(byte4*)&m_pthis->m_cdecl.jmp_mfn[1] = (byte4)mfn-((byte4)&m_pthis->m_cdecl.jmp_mfn+5);
	
	m_pthis->m_cdecl.add_esp[0] = 0x83;
	m_pthis->m_cdecl.add_esp[1] = 0xC4;
	m_pthis->m_cdecl.add_esp[2] = 0x04;

	m_pthis->m_cdecl.jmp_ret[0] = 0x3E;
	m_pthis->m_cdecl.jmp_ret[1] = 0xFF;
	m_pthis->m_cdecl.jmp_ret[2] = 0x25;
	*(byte4*)&m_pthis->m_cdecl.jmp_ret[3] = (byte4)&m_pthis->m_cdecl.ret_addr;
	
	FlushInstructionCache(GetCurrentProcess(),&m_pthis->m_cdecl,sizeof(m_pthis->m_cdecl));
	
	return &m_pthis->m_cdecl;	
}
