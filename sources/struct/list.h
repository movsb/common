#pragma once

/**********************************************************
文件名称:list.h/list.c
文件路径:../list/list.h,../list/list.c
创建时间:2013-1-29,0:23:04
文件作者:女孩不哭
代码备份:http://www.cnblogs.com/nbsofer/archive/2013/02/25/list_entry.html
文件说明:该头文件及实现文件实现了WDK中双向链表的操作函数
	2013-07-13 更新:加入list_remove函数实现移除某一结点
**********************************************************/

typedef struct _list_s{
	struct _list_s* prior;
	struct _list_s* next;
}list_s;

//该宏实现根据结构体中链表的指针得到结构体的指针
//为CONTAINING_RECORD宏的实现
#define list_data(addr,type,member) \
	((type*)(((unsigned char*)addr)-(unsigned long)&(((type*)0)->member)))

#ifdef __cplusplus
extern "C" {
#endif // __cplusplu

int   list_is_empty(list_s* phead);
void  list_init(list_s* phead);
void  list_insert_head(list_s* phead, list_s* plist);
void  list_insert_tail(list_s* phead, list_s* plist);
list_s* list_remove_head(list_s* phead);
list_s* list_remove_tail(list_s* phead);
int list_remove(list_s* phead,list_s* p);

#ifdef __cplusplus
}
#endif // __cplusplus
