#pragma once

namespace Common {
	namespace Window{
		class c_edit : public CWnd
		{
		public:
			c_edit()
				: _bUseDefMenu(true)
			{
			}
			virtual ~c_edit(){
			}

			virtual LPCTSTR GetSuperClassName() const{return WC_EDIT;}
			virtual LPCTSTR GetWindowClassName() const{return "Common" WC_EDIT;}
			virtual bool ResponseDefaultKeyEvent(HWND hwnd, WPARAM wParam) {return false;}

			void clear() {
				::SetWindowText(*this, ""); 
			}
			virtual bool back_delete_char(int n);
			virtual bool append_text(const char* str);
			virtual void set_text(const char* str);

		public: // menu support functions
			virtual bool is_read_only();

		public:
			virtual void limit_text(int sz);

		protected:
			virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;
			HMENU _load_default_menu();


		protected:
			bool _bUseDefMenu;

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

		public:
            virtual void limit_text(int sz) override;
			void set_default_text_fgcolor(COLORREF fg);
			void set_default_text_bgcolor(COLORREF bg);
			bool get_sel_range(int* start = nullptr, int* end = nullptr);
			void do_copy();
			void do_cut();
			void do_paste();
			void do_delete();
			void do_sel_all();

		protected:
			virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;
		
		protected:
			int _deffg;
			int _defbg;
		};
	}
}
