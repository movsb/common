#pragma once

namespace Common{
	class c_file_dlg
	{
	public:
		c_file_dlg();
		virtual bool do_modal(HWND hOwner) = 0;
		void set_title(const char* tt) { _ofn.lpstrTitle = tt; }
		void set_filter(const char* flt){ _ofn.lpstrFilter = flt; }
		void set_flag(DWORD dwflg) { _ofn.Flags = dwflg; }
		char* get_buffer() { return _buffer; }



	protected:
		char _buffer[64 * (1 << 10)]; // 64KB max file
		OPENFILENAME _ofn;
	};

	class c_file_open_dlg : public c_file_dlg
	{
	public:
		c_file_open_dlg();
		virtual bool do_modal(HWND hOwner) override;
	};

	class c_file_save_dlg : public c_file_dlg
	{
	public:
		c_file_save_dlg();
		virtual bool do_modal(HWND hOwner) override;
	};
}
