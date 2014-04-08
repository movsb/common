#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_memory(void);

//用于Manage_mem的动作参数
enum{MANMEM_INITIALIZE,MANMEM_INTSERT,MANMEM_REMOVE,MANMEM_FREE};

//#ifdef _DEBUG
#define GET_MEM(size) memory.get_mem_debug(size,__THIS_FILE__,__LINE__)
//#else
//#define GET_MEM(size) memory.get_mem(size)
//#endif

struct memory_s{
	void (*manage_mem)(int what,void* pv);
//#ifdef _DEBUG
	void* (*get_mem_debug)(size_t size,char* file,int line);
//#else
//	void* (*get_mem)(size_t size);
//#endif
	void (*free_mem)(void** ppv,char* prefix);
};

#ifndef __MEMORY_C__
	extern struct memory_s memory;
#else
#undef __MEMORY_C__
struct memory_s memory;



void manage_mem(int what,void* pv);
//#ifdef _DEBUG
void* get_mem_debug(size_t size,char* file,int line);
//#else
//void* get_mem(size_t size);
//#endif
void free_mem(void** ppv,char* prefix);

#endif

#ifdef __cplusplus
}
#endif

#endif//!__MEMORY_H__
