#pragma once

namespace Common{
	class i_notifier;

	class c_memory
	{
	public:
		c_memory();
		~c_memory();
		void set_notifier(i_notifier* pn);
		void* get(size_t size,char* file,int line);
		void free(void** ppv,char* prefix);

	protected:
		void _insert(list_s* p);
		int _remove(list_s* p);
		void _delect_leak();

	protected:
		i_notifier* _notifier;
		list_s _head;
		CRITICAL_SECTION _cs;
	};

	extern c_memory memory;
}

#define GET_MEM(size) memory.get(size,__THIS_FILE__,__LINE__)
