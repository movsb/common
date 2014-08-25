#ifndef __COM_UTILS_H__
#define __COM_UTILS_H__
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

enum newline_type {NLT_CR,NLT_LF,NLT_CRLF};

#define __ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#ifdef __cplusplus
extern "C" {
#endif


void init_utils(void);

struct utils_s{
	int (*msgbox)(HWND hOwner,UINT msgicon, char* caption, char* fmt, ...);
	void (*msgerr)(HWND hOwner,char* prefix);
	char* (*get_file_name)(char* title, char* filter, int action, int* opentype);
	int (*set_clip_data)(char* str);
	unsigned int (*str2hex)(char* str, unsigned char** ppBuffer,unsigned int buf_size);
	char* (*hex2str)(unsigned char* hexarray, int* length, int linecch, int start, char* buf, int buf_size);
	char* (*hex2chs)(unsigned char* hexarray,int length,char* buf,int buf_size, enum newline_type nlt);
	void (*center_window)(HWND hWnd, HWND hWndOwner);
	void (*assert_expr)(void* pv,char* str);
	int (*wstr2lstr)(char* src);
	int (*check_chs)(unsigned char* ba, int cb);
	unsigned int (*remove_string_return)(char* str);
	unsigned int (*remove_string_linefeed)(char* str);
	unsigned int (*parse_string_escape_char)(char* str);
	unsigned int (*eliminate_control_char)(char* str);
};

#ifndef __UTILS_C__
	extern struct utils_s utils;
#else
#undef __UTILS_C__
	
int msgbox(HWND hOwner,UINT msgicon, char* caption, char* fmt, ...);
void msgerr(HWND hOwner,char* prefix);
char* get_file_name(char* title, char* filter, int action, int* opentype);
int set_clip_data(char* str);
unsigned int str2hex(char* str, unsigned char** ppBuffer,unsigned int buf_size);
char* hex2str(unsigned char* hexarray, int* length, int linecch, int start, char* buf, int buf_size);
char* hex2chs(unsigned char* hexarray,int length,char* buf,int buf_size, enum newline_type nlt);
void center_window(HWND hWnd, HWND hWndOwner);
void myassert(void* pv,char* str);
int wstr2lstr(char* src);
int check_chs(unsigned char* ba, int cb);
unsigned int remove_string_return(char* str);
unsigned int remove_string_linefeed(char* str);
unsigned int parse_string_escape_char(char* str);
unsigned int eliminate_control_char(char* str);
#endif


#ifdef __cplusplus
}
#endif


#endif//!__COM_UTILS_H__
