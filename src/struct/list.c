#include <stdio.h>
#include "list.h"

/**************************************************
函  数:list_is_empty@4
功  能:判断双向链表是否为空
参  数:phead - 双向链表头指针
返回值:非零表示为空,零表示不为空
说  明:
**************************************************/
int list_is_empty(list_s* phead)
{
	return phead->next == phead;
}

/**************************************************
函  数:list_init@4
功  能:初始化双向链表为空
参  数:phead - 链表头指针
返回值:(无)
说  明:
**************************************************/
void list_init(list_s* phead)
{
	//使前后指针都指向自己(头结点)
	//即为空链表
	phead->prior = phead;
	phead->next = phead;
}

/**************************************************
函  数:list_insert_head@8
功  能:从双向链表头部插入结点
参  数:	phead - 链表头指针
		plist - 待插入的链表结点指针
返回值:(无)
说  明:
**************************************************/
void list_insert_head(list_s* phead, list_s* plist)
{
	//得到第1个结点的指针
	//为空链表时也满足
	list_s* first = phead->next;
	//在头结点和第1个结点之间插入时需要:
	//	头结点的prior不变;头结点的next指向新结点
	//	新结点的prior指向头结点;新结点的next指向第1个结点
	//	第1个结点的prior指向新结点;第1个结点的next不变
	phead->next = plist;
	plist->prior = phead;
	plist->next = first;
	first->prior = plist;
}

/**************************************************
函  数:list_insert_tail@8
功  能:从双向链表尾部插入结点
参  数:	phead - 链表头指针
		plist - 待插入的链表结点指针
返回值:(无)
说  明:
**************************************************/
void list_insert_tail(list_s* phead, list_s* plist)
{
	//得到最后一个结点的指针
	//为空链表时也满足
	list_s* last = phead->prior;
	//在最后一个结点和头结点之间插入时需要:
	//	最后一个结点的next指向新结点;最后一个结点的prior不变
	//	新结点的prior指向最后一个结点;新结点的next指向头结点
	//	头结点的next不变;头结点的prior指向新结点
	last->next = plist;
	plist->prior = last;
	plist->next = phead;
	phead->prior = plist;
}

/**************************************************
函  数:list_remove_head@4
功  能:从双向链表头部移除结点
参  数:phead - 链表头指针
返回值:被移除的结点指针
	若链表为空,返回NULL
说  明:
**************************************************/
list_s* list_remove_head(list_s* phead)
{
	list_s* second = NULL;
	list_s* removed = NULL;
	if(list_is_empty(phead))
		return NULL;
	//需要移除第1个结点,则应该先保存第2个结点
	//若不存在第2个结点也满足条件(此时second即为头结点)
	second = phead->next->next;
	removed = phead->next;
	//移除第1个结点需要:
	//	头结点的next指向第2个结点(若不存在,则指向自己);头结点的prior不变
	//	第2个结点的prior指向头结点;第2个结点的next不变
	phead->next = second;
	second->prior = phead;
	return removed;
}

/**************************************************
函  数:list_remove_tail@4
功  能:从双向链表尾部移除结点
参  数:phead - 链表头指针
返回值:被移除的结点指针
	若链表为空,返回NULL
说  明:
**************************************************/
list_s* list_remove_tail(list_s* phead)
{
	list_s* second_last = NULL;
	list_s* removed = NULL;
	if(list_is_empty(phead))
		return NULL;
	//需要移除最后一个结点,需要保存倒数第2个结点指针
	//若不存在倒数第2个(仅一个结点时),倒数第2个就是头结点
	second_last = phead->prior->prior;
	removed = phead->prior;
	//移除一个结点需要
	//	倒数第2个结点的next指向头结点,prior不变
	//	头结点的prior指向倒数第2个结点,next不变
	second_last->next = phead;
	phead->prior = second_last;
	return removed;
}

/**************************************************
函  数:list_remove
功  能:移除结点指针为p的结点,并不释放节点内存,也没法去释放
参  数:	p - 待移除的结点
		phead - 链接头结点
返  回:	0 - 无此结点
		1 - 成功
		2 - 链表为空
说  明:
**************************************************/
int list_remove(list_s* phead,list_s* p)
{
	if(!list_is_empty(phead)){
		list_s* node = NULL;
		for(node=phead->next; node!=phead; node=node->next){
			if(node == p){
				//移除该结点需要:
				// 将当前结点的上一个结点:
				//		next指向当前结点的next
				// 将当前结点的下一个结点:
				//		prior指向当前结点的上一个结点
				node->prior->next = node->next;
				node->next->prior = node->prior;
				return 1;
			}
		}
		return 0;
	}else{
		return 2;
	}
}
