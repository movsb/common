#include <Windows.h>
#define __MEMORY_C__
#include "memory.h"
#include "list.h"
#include "../utils.h"
#include "../debug.h"
#include "../about.h"
#include "../msg.h"

/**********************************************************
文件名称:memory.c/memory.h
文件路径:./common/struct/memory.c/.h
创建时间:2013-07-14,09:55
文件作者:女孩不哭
文件说明:内存管理
**********************************************************/

static char* __THIS_FILE__ = __FILE__;

void init_memory(void)
{
	memory.manage_mem = manage_mem;
//#ifdef _DEBUG
	memory.get_mem_debug = get_mem_debug;
//#else
//	memory.get_mem = get_mem;
//#endif
	memory.free_mem = free_mem;
}

/**************************************************
函  数:get_mem@4
功  能:分配内存,并清零
参  数:size-待分配的大小
返回值:内存区指针
说  明:
	2013-07-13:修改分配方式,方便调试程序
**************************************************/
#pragma pack(push,1)
typedef struct {
	unsigned char sign_head[2];
//#ifdef _DEBUG
	char* file;
	int line;
//#else

//#endif
	size_t size;
	list_s entry;
	//unsigned char buffer[1];
}common_mem_context;
typedef struct {
	unsigned char sign_tail[2];
}common_mem_context_tail;
#pragma pack(pop)

/**************************************************
函  数:manage_mem
功  能:我的内存分配管理函数,管理分配的内存,释放的内存
参  数:	pv - &list_entry
返  回:
说  明:
**************************************************/
void manage_mem(int what,void* pv)
{
	static list_s list_head;
	static CRITICAL_SECTION critical_section;

	if(what==MANMEM_INTSERT || what==MANMEM_REMOVE){
		__try{
			EnterCriticalSection(&critical_section);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
			MessageBox(NULL,"内存未正确初始化!",NULL,MB_ICONERROR);
		}
	}

	switch(what)
	{
	case MANMEM_INITIALIZE:
		list->init(&list_head);
		InitializeCriticalSection(&critical_section);
		return;
	case MANMEM_INTSERT:
		list->insert_tail(&list_head,(list_s*)pv);
		break;
	case MANMEM_REMOVE:
		switch(list->remove(&list_head,(list_s*)pv))
		{
		case 1:
			break;
		case 0:
			utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"无此结点:%p",pv);
			break;
		case 2:
			utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"内存链表为空!");
			break;
		}
		break;
	case MANMEM_FREE:
		debug_out(("检测内存泄漏...\n"));
		if(!list->is_empty(&list_head)){
			int i=1;
#ifdef _DEBUG
			utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"发现未被释放的内存!\n\n请向作者提交内存分配信息~");
#endif
			//由于free_mem会移除链表结点,所以这里只能遍历,不能移除
			while(!list->is_empty(&list_head)){
				list_s* node = list_head.next;
				common_mem_context* pc = list_data(node,common_mem_context,entry);
				void* user_ptr = (void*)((unsigned char*)pc+sizeof(*pc));
#ifdef _DEBUG
				utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,NULL,
					"内存结点%d:\n\n"
					"以下是内存分配信息:\n\n"
					"分配大小:%u\n"
					"来自文件:%s\n"
					"文件行号:%d\n",
					i++,pc->size,pc->file,pc->line);
#endif
				memory.free_mem(&user_ptr,"MANMEM_FREE");
			}
			list->init(&list_head);
			DeleteCriticalSection(&critical_section);
			return;
		}
		return;
	}
	LeaveCriticalSection(&critical_section);
}


//#ifdef _DEBUG
	void* get_mem_debug(size_t size,char* file,int line)
//#else
//	void* get_mem(size_t size)
//#endif
{
	size_t all = sizeof(common_mem_context)+size+sizeof(common_mem_context_tail);
	void* pv = malloc(all);
	common_mem_context* pc = (common_mem_context*)pv;
	void* user_ptr = (unsigned char*)pc+sizeof(*pc);
	common_mem_context_tail* pct = (common_mem_context_tail*)((unsigned char*)user_ptr+size);
	if(!pv){
		utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"内存分配错误");
		return NULL;
	}

	memset(pv,0,all);

	pc->sign_head[0] = 'J';
	pc->sign_head[1] = 'J';

	pc->size = size;

//#ifdef _DEBUG
	pc->file = file;
	pc->line = line;
	//debug_out(("分配内存:%u 字节\n来自文件:%s\n文件行号:%d\n\n",pc->size,pc->file,pc->line));
//#endif

	pct->sign_tail[0] = 'J';
	pct->sign_tail[1] = 'J';

	manage_mem(MANMEM_INTSERT,&pc->entry);


	return user_ptr;
}

/**************************************************
函  数:free_mem@8
功  能:释放内存区域,异常处理
参  数:void** ppv:user_ptr指针,char* prefix:说明
返回值:
说  明:
**************************************************/
void free_mem(void** ppv,char* prefix)
{
	common_mem_context* pc = NULL;
	common_mem_context_tail* pct = NULL;
	if(ppv==NULL || *ppv==NULL)
	{
//#ifdef _DEBUG
		utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,NULL,"memory.free_mem 释放空指针, 来自:%s",prefix);
//#endif
		return;
	}

	__try{
		pc = (common_mem_context*)((size_t)*ppv-sizeof(*pc));
		pct = (common_mem_context_tail*)((unsigned char*)pc + sizeof(common_mem_context) + pc->size);

		if((pc->sign_head[0]=='J'&&pc->sign_head[1]=='J') &&
			pct->sign_tail[0]=='J'&&pct->sign_tail[1]=='J')
		{
			manage_mem(MANMEM_REMOVE,&pc->entry);
			free(pc);
			*ppv = NULL;			
		}else{

//#ifdef _DEBUG
			manage_mem(MANMEM_REMOVE,&pc->entry);
			utils.msgbox(msg.hWndMain,MB_ICONERROR,"debug error",
				"待释放内存签名不正确!\n\n"
				"文件:%s\n"
				"行数:%d",pc->file,pc->line);
//#else
//			utils.msgbox(msg.hWndMain,MB_ICONERROR,"debug error","待释放内存签名不正确!");
//#endif
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
//#ifdef _DEBUG
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,
			"%s:指针被错误地释放,请报告异常!\n\n"
			"文件:%s\n"
			"行数:%d",prefix?prefix:"<null-function-name>",pc->file,pc->line);
//#else
//		utils.msgbox(msg.hWndMain,MB_ICONERROR,"debug error","%s:指针被错误地释放,请报告异常!",prefix?prefix:"<null-function-name>");
//#endif
	}
}

