#pragma once

namespace Common {
	template<class T>
	class c_ptr_array
	{
	public:
		c_ptr_array()
			: m_pp(0)
			, m_cnt(0)
			, m_allocated(0)
		{

		}
		virtual ~c_ptr_array()
		{
			empty();
		}

	public:
		void empty()
		{
			if(m_pp){
				::free(m_pp);
			}
			m_pp = 0;
			m_cnt = 0;
			m_allocated = 0;
		}

		int find(T* pv) const
		{
			for(int i=0; i<m_cnt; i++){
				if(m_pp[i] == pv){
					return i;
				}
			}
			return -1;
		}

		bool add(T* pv)
		{
			assert(find(pv)==-1);
			if(++m_cnt > m_allocated){
				int n = m_allocated * 2;
				if(!n) n = 1;

				T** pp = static_cast<T**>(::realloc(m_pp, n * sizeof(void*)));
				if(pp){
					m_allocated = n;
					m_pp = pp;
				}
				else{
					--m_cnt;
					return false;
				}
			}
			m_pp[m_cnt-1] = pv;
			return true;
		}

		bool remove(T* pv)
		{
			int i = find(pv);
			if(i == -1){
				assert(0);
				return false;
			}
			else{
				--m_cnt;
				::memmove(m_pp+i, m_pp+i+1, (m_cnt-i)*sizeof(void*));
				return true;
			}
		}

		int size() const
		{
			return m_cnt;
		}

		T* getat(int i) const
		{
			assert(i>=0 && i<m_cnt);
			return m_pp[i];
		}

		T* operator[](int i) const
		{
			return getat(i);
		}

	protected:
		T** m_pp;
		int m_cnt;
		int m_allocated;
	};

	template <int pre_size, int granularity>
	class c_byte_array
	{
	public:
		c_byte_array()
			: _data(0)
			, _pos(0)
		{
			assert(pre_size >= 0);
			assert(granularity > 0);
			_size = pre_size;
			_granularity = granularity;

			if(_size > 0){
				_data = new unsigned char[_size];
			}
		}
		~c_byte_array()
		{
			empty();
		}

		void empty()
		{
			if(_data){
				delete[] _data;
				_data = NULL;
			}
			_pos = 0;
			_size = 0;
		}

		void* get_data() const
		{
			return _data;
		}

		int get_size() const
		{
			return _pos;
		}

		void append(const unsigned char* ba, int cb)
		{
			if(cb > get_space_left())
				inc_data_space(cb);
			memcpy(_data+get_space_used(), ba, cb);
			_pos += cb;
		}

		void append_char(const unsigned char c)
		{
			return append(&c, 1);
		}

	protected:
		int get_space_left()
		{
			return _size-1 - _pos + 1;
		}

		int get_space_used()
		{
			return _pos;
		}

		bool inc_data_space(int addition)
		{
			int left = addition - get_space_left();
			int n_blocks = left / _granularity;
			int n_remain = left - n_blocks * _granularity;

			if(n_remain) ++n_blocks;

			int new_size = _size + n_blocks * _granularity;
			unsigned char* p = new unsigned char[new_size];

			memcpy(p, _data, get_space_used());

			if(_data) delete[] _data;
			_data = p;
			_size = new_size;

			return true;
		}

	protected:
		unsigned char* _data;
		int _size;
		int _pos;
		int _granularity;
	};

	// 观察者模式
	class i_observer
	{
	public:
		virtual bool do_event() = 0;
	};

	class i_observable
	{
	public:
		virtual operator i_observable*() = 0;
		virtual void empty() = 0;
		virtual int find(i_observer* ob) = 0;
		virtual bool add(i_observer* ob) = 0;
		virtual bool add(std::function<bool()> doevt) = 0;
		virtual bool remove(i_observer* ob) = 0;
		virtual int size() = 0;
		virtual i_observer* getat(int i) const = 0;
		virtual i_observer* operator[](int i) const = 0;
	};

	class c_observable : public i_observable
	{
	public:
		virtual operator i_observable*(){
			return static_cast<i_observable*>(this);
		}
		virtual void empty(){
			_obs.empty();
		}
		virtual int find(i_observer* ob){
			return _obs.find(ob);
		}
		virtual bool add(i_observer* ob){
			return _obs.add(ob);
		}
		virtual bool add(std::function<bool()> doevt){
			_fobs.push_back(doevt);
			return true;
		}
		virtual bool remove(i_observer* ob){
			return _obs.remove(ob);
		}
		virtual int size(){
			return _obs.size();
		}
		virtual i_observer* getat(int i) const{
			return _obs.getat(i);
		}
		virtual i_observer* operator[](int i) const{
			return _obs[i];
		}

	public:
		bool call_observers()
		{
			for (int i = 0; i < size(); i++){
				if (getat(i)->do_event()){
					return false;
				}
			}
			for (auto& ob : _fobs){
				if (ob()){
					return false;
				}
			}
			return true;
		}

	protected:
		c_ptr_array<i_observer> _obs;
		std::vector<std::function<bool()>> _fobs;
	};

	class c_critical_locker
	{
	public:
		c_critical_locker()
		{
			::InitializeCriticalSection(&_cs);
		}
		~c_critical_locker()
		{
			::DeleteCriticalSection(&_cs);
		}

		void lock()
		{
			::EnterCriticalSection(&_cs);
		}

		void unlock()
		{
			::LeaveCriticalSection(&_cs);
		}

		bool try_lock()
		{
			return !!::TryEnterCriticalSection(&_cs);
		}

	private:
		CRITICAL_SECTION _cs;
	};

    class c_lock_guard
    {
    public:
        c_lock_guard(c_critical_locker& lock_) 
            : _lock(lock_)
        {
            _lock.lock();
        }

        ~c_lock_guard() {
            _lock.unlock();
        }

    private:
        c_critical_locker& _lock;
    };

	class c_text_formatting
	{
	public:
		enum newline_type {NLT_CR,NLT_LF,NLT_CRLF};

		/**************************************************
		函  数:remove_string_cr
		功  能:移除字符串中的 '\r'
		参  数:str - 待剔除回车换行的字符串
		返  回:结果字符串的长度
		说  明:
		**************************************************/
		static unsigned int remove_string_cr(char* str);

		/**************************************************
		函  数:remove_string_crlf
		功  能:移除字符串中的 '\r','\n'
		参  数:str - 待剔除'\r','\n'的字符串
		返  回:结果字符串的长度
		说  明:
		**************************************************/
		static unsigned int remove_string_crlf(char* str);

		/**************************************************
		函  数:remove_string_lf
		功  能:移除字符串中的 '\n'
		参  数:str - 待剔除'\n'的字符串
		返  回:结果字符串的长度
		说  明: 
		**************************************************/	
		static unsigned int remove_string_lf(char* str);

		/**************************************************
		函  数:parse_string_escape_char
		功  能:解析掉字符串中的转义字符
		参  数:str - 待解析的字符串
		返  回:
			1.若解析全部成功:
				最高位为1,其余位为解析后的字符串长度
			2.若解析时遇到错误:
				最高位为0,其余位为解析直到错误时的长度
		说  明:
			1.支持的字符型转义字符:
				\r,\n,\t,\v,\a,\b,\\
			2.支持的16进制转义字符格式:
				\x?? - 其中一个问号代表一个16进制字符, 不可省略其一,
				必需保证4个字符的格式
			3.'?',''','"', 等print-able字符不需要转义
			4.源字符串会被修改 - 一直不习惯用const修饰, 该注意下了
			5.支持的8进制转义字符格式:
				\??? - 其中一个问号代表一个8进制字符, 1-3位, 最大为377
		**************************************************/
		static unsigned int parse_string_escape_char(char* str);

		/**************************************************
		函  数:str2hex
		功  能:转换16进制字符串到16进制值数组
		参  数:
			str:指向包含16进制的字符串
			ppBuffer:unsigned char**,用来保存转换后的结果的缓冲区,可指定默认缓冲区
			buf_size:若传入默认缓冲区, 指定默认缓冲区的大小(字节)
		返回值:
			成功:最高位为1,低31位表示得到的16进制数的个数
			失败:最高位为0,低31位表示已经解析的字符串的长度
		说  明:如果换用sscanf应该要快些,不过刚开始写的时候没考虑到
			2013-04-07更新:更改为词法解析版,大大增加16进制内容的灵活性
			2013-07-27更新:增加默认缓冲区,注意:
					ppBuffer应该指定一个指针变量的地址,该指针变量的值为默认缓冲区或NULL
					若指定默认缓冲区,那么buf_size为缓冲区容量
		***************************************************/
		static unsigned int str2hex(char* str, unsigned char** ppBuffer,unsigned int buf_size);

		/*************************************************
		函  数:hex2str
		功  能:转换16进制值数组到16进制字符串
		参  数:
			hexarray:16进制数组
			*length:16进制数组长度
			linecch:每行的16进制的个数,为0表示不换行
			start:开始于第几个16进制序列
			buf:默认空间,如果空间大小合适,则采用此空间
			buf_size:空间大小
		返回值:
			成功:字符串指针(如果不是默认缓冲区,需要手动释放)
			失败:NULL
			*length 返回返回字符串的长度
		说  明:
			2013-03-05:修正, 由于可能添加较频繁,但每次的数据又很少,
		所以现在可以传入用户定义的缓冲来保存数据了,若返回值==buf,
		说明用户空间被使用
			2013-03-10:以前少加了一句:*pb='\0'; 
				导致接收区总是显示乱码(数据量不正确),找了好久才发现....
		**************************************************/
		static char* hex2str(unsigned char* hexarray, int* length, int linecch, int start, char* buf, int buf_size, enum newline_type nlt);

		/**************************************************
		函  数:hex2chs
		功  能:转换16进制数组到字符字符串
		参  数:	hexarray - 16进制数组
				length - 长度
				buf - 默认缓冲空间
				buf_size - 默认空间大小
		返回值:字符串
		说  明:2013-03-10:作了很多修改,大量减少丢包
		2013-03-23 修正:
			用C语言的人们都都习惯于使用'\n'作为换行符,我也这样使用,
		但偏偏Windows的编辑框以'\r\n'作为换行符,没有办法

		2014-07-07 修正:
			现在做如下统一换行要求:
				① \r 后面没有 \n
				② \n 
				③ \r\n
				④ \r\n\r
				如果出现上面四种情况, 均作为一个换行符处理
			突然发现, 其实在这里可以全部处理掉所有(最后一个除外)的'\0'.
		2014-08-08:
			为了防止edit乱码, 此处以3个'\0'作为结束

		2014-08-13:
			RichEdit仅使用'\r'作为换行, 多添加一个参数
		**************************************************/
		static char* hex2chs(unsigned char* hexarray,int length,char* buf,int buf_size, enum newline_type nlt);
	};

	void set_clipboard_data(const char* str);

	int read_integer(const char* str, int* pi);
	unsigned char val_from_char(char c);
	int char_oct_from_chars(const char* str, unsigned char* poct);

	void split_string(std::vector<std::string>* vec, const char* str, char delimiter);
}


#define __ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
