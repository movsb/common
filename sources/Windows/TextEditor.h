#pragma once

namespace Common {
	namespace Window{
		class c_edit : public CWnd
		{
		public:
			c_edit(){
				::InitializeCriticalSection(&_cs);
			}
			virtual ~c_edit(){
				::DeleteCriticalSection(&_cs);
			}

			void lock() { ::EnterCriticalSection(&_cs); }
			void unlock() { ::LeaveCriticalSection(&_cs); }

			virtual LPCTSTR GetSuperClassName() const{return WC_EDIT;}
			virtual LPCTSTR GetWindowClassName() const{return "Common" WC_EDIT;}
			virtual bool ResponseDefaultKeyEvent(HWND hwnd, WPARAM wParam) {return false;}

			void clear() {
				lock();
				::SetWindowText(*this, ""); 
				unlock();
			}
			virtual bool back_delete_char(int n);
			virtual bool append_text(const char* str);
			virtual void set_text(const char* str);

		public:
			virtual void limit_text(int sz);

		protected:
			CRITICAL_SECTION _cs;
		};

		class c_rich_edit : public c_edit
		{
		public:
			c_rich_edit()
				: _deffg(30)
				, _defbg(47)
			{}

			virtual LPCTSTR GetSuperClassName() const {return RICHEDIT_CLASS;}
			virtual LPCTSTR GetWindowClassName() const{return "Common" RICHEDIT_CLASS;}
			virtual bool back_delete_char(int n);
			virtual bool append_text(const char* str);
			virtual bool apply_linux_attributes(char* attrs);
			virtual bool apply_linux_attribute_m(int attr);

		protected:
			int _deffg;
			int _defbg;
		};
	}
}
