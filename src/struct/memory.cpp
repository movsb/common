#include "StdAfx.h"
#include "list.h"
#include "../debug.h"

/**********************************************************
文件名称:memory.c/memory.h
文件路径:./common/struct/memory.c/.h
创建时间:2013-07-14,09:55
文件作者:女孩不哭
文件说明:内存管理
**********************************************************/

static char* __THIS_FILE__ = __FILE__;

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
	char* file;
	int line;
	size_t size;
	list_s entry;
}common_mem_context;
typedef struct {
	unsigned char sign_tail[2];
}common_mem_context_tail;
#pragma pack(pop)

namespace Common{

	c_memory memory;

	void* c_memory::get( size_t size,char* file,int line )
	{
		size_t all = sizeof(common_mem_context)+size+sizeof(common_mem_context_tail);
		void* pv = malloc(all);
		common_mem_context* pc = (common_mem_context*)pv;
		void* user_ptr = (unsigned char*)pc+sizeof(*pc);
		common_mem_context_tail* pct = (common_mem_context_tail*)((unsigned char*)user_ptr+size);
		if(!pv){
			_notifier->msgbox(MB_ICONERROR,NULL,"内存分配错误");
			return NULL;
		}

		memset(pv,0,all);

		pc->sign_head[0] = 'J';
		pc->sign_head[1] = 'J';

		pc->size = size;
		pc->file = file;
		pc->line = line;

		pct->sign_tail[0] = 'J';
		pct->sign_tail[1] = 'J';

		_insert(&pc->entry);


		return user_ptr;
	}

	void c_memory::free( void** ppv,char* prefix )
	{
		common_mem_context* pc = NULL;
		common_mem_context_tail* pct = NULL;
		if(ppv==NULL || *ppv==NULL)
		{
			_notifier->msgbox(MB_ICONEXCLAMATION,NULL,"memory.free_mem 释放空指针, 来自:%s",prefix);
			return;
		}

		__try{
			pc = (common_mem_context*)((size_t)*ppv-sizeof(*pc));
			pct = (common_mem_context_tail*)((unsigned char*)pc + sizeof(common_mem_context) + pc->size);

			if((pc->sign_head[0]=='J'&&pc->sign_head[1]=='J') &&
				pct->sign_tail[0]=='J'&&pct->sign_tail[1]=='J')
			{
				_remove(&pc->entry);
				::free(pc);
				*ppv = NULL;			
			}else{
				_remove(&pc->entry);
				_notifier->msgbox(MB_ICONERROR,"debug error",
					"待释放内存签名不正确!\n\n"
					"文件:%s\n"
					"行数:%d",pc->file,pc->line);
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
			_notifier->msgbox(MB_ICONERROR,"",
				"%s:指针被错误地释放,请报告异常!\n\n"
				"文件:%s\n"
				"行数:%d",prefix?prefix:"<null-function-name>",pc->file,pc->line);
		}
	}

	c_memory::c_memory()
	{
		list_init(&_head);
		InitializeCriticalSection(&_cs);
	}

	c_memory::~c_memory()
	{
		_delect_leak();
		list_init(&_head);
		DeleteCriticalSection(&_cs);
	}

	void c_memory::set_notifier( i_notifier* pn )
	{
		_notifier = pn;
	}

	void c_memory::_insert( list_s* p )
	{
		::EnterCriticalSection(&_cs);
		list_insert_tail(&_head, p);
		::LeaveCriticalSection(&_cs);
	}

	int c_memory::_remove( list_s* p )
	{
		int r;
		::EnterCriticalSection(&_cs);
		r = list_remove(&_head, p);
		::LeaveCriticalSection(&_cs);
		return r;
	}

	void c_memory::_delect_leak()
	{
		if(!list_is_empty(&_head)){
			int i=1;
#ifdef _DEBUG
			_notifier->msgbox(MB_ICONERROR,NULL,"发现未被释放的内存!\n\n请向作者提交内存分配信息~");
#endif
			//由于free_mem会移除链表结点,所以这里只能遍历,不能移除
			while(!list_is_empty(&_head)){
				list_s* node = _head.next;
				common_mem_context* pc = list_data(node,common_mem_context,entry);
				void* user_ptr = (void*)((unsigned char*)pc+sizeof(*pc));
#ifdef _DEBUG
				_notifier->msgbox(MB_ICONEXCLAMATION,NULL,
					"内存结点%d:\n\n"
					"以下是内存分配信息:\n\n"
					"分配大小:%u\n"
					"来自文件:%s\n"
					"文件行号:%d\n",
					i++,pc->size,pc->file,pc->line);
#endif
				free(&user_ptr,"MANMEM_FREE");
			}
		}
	}

}
