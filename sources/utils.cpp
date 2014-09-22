#include "StdAfx.h"
#include "utils.h"
#include "msg.h"
#include "about.h"
#include "debug.h"
#include "struct/memory.h"
#include "comm.h"
#include "../res/resource.h"

static char* __THIS_FILE__ = __FILE__;

namespace Common {
	unsigned int c_text_formatting::remove_string_crlf( char* str )
	{
		char* p1 = str;
		char* p2 = str;

		while(*p2){
			if(*p2=='\r' || *p2=='\n'){
				p2++;
			}else{
				*p1++ = *p2++;
			}
		}
		*p1 = '\0';
		return (unsigned int)p1-(unsigned int)str;
	}

	unsigned int c_text_formatting::remove_string_cr(char* str)
	{
		char* p1 = str;
		char* p2 = str;

		while (*p2){
			if (*p2 == '\r'){
				p2++;
			}
			else{
				*p1++ = *p2++;
			}
		}
		*p1 = '\0';
		return (unsigned int)p1 - (unsigned int)str;
	}

	unsigned int c_text_formatting::remove_string_lf( char* str )
	{
		char* p1 = str;
		char* p2 = str;

		while(*p2){
			if(*p2=='\n'){
				p2++;
			}else{
				*p1++ = *p2++;
			}
		}
		*p1 = '\0';
		return (unsigned int)p1-(unsigned int)str;
	}

	unsigned char val_from_char(char c)
	{
		if(c>='0' && c<='9') return c-'0';
		else if(c>='a' && c<='f') return c-'a'+10;
		else if(c>='A' && c<='F') return c-'A'+10;
		else return 0;
	}

	int char_oct_from_chars(const char* str, unsigned char* poct)
	{
		unsigned char oct = 0;
		int i;

		for(i=0; i<3 && (*str>='0' && *str<='7'); i++,str++){
			oct *= 8;
			oct += *str-'0';
		}

		*poct = oct;
		return i;
	}

	int read_integer(const char* str, int* pi)
	{
		int r = 0;
		const char* p = str;

		while (*p >= '0' && *p <= '9'){
			r *= 10;
			r += *p - '0';
			p++;
		}

		*pi = r;
		return (int)p - (int)str;
	}

	unsigned int c_text_formatting::parse_string_escape_char( char* str )
	{
		char* p1 = str;
		char* p2 = str;

		while(*p2){
			if(*p2 == '\\'){
				p2++;
				switch(*p2)
				{
				case '\\':*p1++ = '\\';p2++;break;
				case '\'':*p1++ = '\''; p2++; break;
				case '\"':*p1++ = '\"'; p2++; break;
				case 'b':*p1++  = '\b';p2++;break;
				case 'a':*p1++  = '\a';p2++;break;
				case 'v':*p1++  = '\v';p2++;break;
				case 't':*p1++  = '\t';p2++;break;
				case 'n':*p1++  = '\n';p2++;break;
				case 'r':*p1++  = '\r';p2++;break;
				case 'x'://检测是否为2个16进制字符
					{
						p2++;
						if(*p2 && *(p2+1)){
							if(isxdigit(*p2) && isxdigit(*(p2+1))){
								unsigned char hex = val_from_char(*p2);
								hex = (hex << 4) + val_from_char(*(p2+1));
								*(unsigned char*)p1 = hex;
								p1++;
								p2 += 2;
							}else{
								goto _error;
							}
						}else{
							goto _error;
						}
						break;
					}
				case '\0':
					goto _error;
					break;
				default:
					{
						// 8进制判断
						if(*p2>='0' && *p2<='7'){
							p2 += char_oct_from_chars(p2, (unsigned char*)p1);
							p1 ++;
							break;
						}
						goto _error;
					}
				}
			}else{
				*p1++ = *p2++;
			}
		}
		*p1 = '\0';
		return 0x80000000|(unsigned int)p1-(unsigned int)str;

_error:
		return (unsigned int)p2-(unsigned int)str & 0x7FFFFFFF;
	}

	unsigned int c_text_formatting::str2hex( char* str, unsigned char** ppBuffer,unsigned int buf_size )
	{
		enum{S2H_NULL,S2H_SPACE,S2H_HEX,S2H_END};
		unsigned char hex=0;			//用来保存解析的单个16进制值
		unsigned int count=0;			//保存解析的16进制的个数
		unsigned char* hexarray;		//保存转换后的结果
		unsigned char* pba;				//用来向hexarray中写数据
		unsigned char* pp = (unsigned char*)str;	//待解析的字符串

		int flag_last=S2H_NULL,flag;				//词法解析用到的标记位

		if(str==NULL) return 0;
		//由于是2个字符+若干空白组成一个16进制, 所以最多不可能超过(strlen(str)/2)
		if(*ppBuffer && buf_size>=strlen(str)/2){
			hexarray = *ppBuffer;
		}else{
			hexarray = (unsigned char*)GET_MEM(strlen(str)/2);
			if(hexarray == NULL){
				*ppBuffer = NULL;
				return 0;
			}else{
				//放到最后,判断是否需要释放
				//*ppBuffer = hexarray;
			}
		}
		pba = hexarray;

		for(;;){
			if(*pp == 0) 
				flag = S2H_END;
			else if(isxdigit(*pp)) 
				flag = S2H_HEX;
			else if(*pp==0x20||*pp==0x09||*pp=='\r'||*pp=='\n')
				flag = S2H_SPACE;
			else{
				//printf("非法字符!\n");
				goto _parse_error;
			}

			switch(flag_last)
			{
			case S2H_HEX:
				{
					if(flag==S2H_HEX){
						hex <<= 4;
						if(isdigit(*pp)) hex += *pp-'0';
						else hex += (*pp|0x20)-87;
						*pba++ = hex;
						count++;
						flag_last = S2H_NULL;
						pp++;
						continue;
					}else{
						//printf("不完整!\n");
						goto _parse_error;
					}
				}
			case S2H_SPACE:
				{
					if(flag == S2H_SPACE){
						pp++;
						continue;
					}else if(flag == S2H_HEX){
						if(isdigit(*pp)) hex = *pp-'0';
						else hex = (*pp|0x20)-87;  //'a'(97)-->10
						pp++;
						flag_last = S2H_HEX;
						continue;
					}else if(flag == S2H_END){
						goto _exit_for;
					}
				}
			case S2H_NULL:
				{
					if(flag==S2H_HEX){
						if(isdigit(*pp)) hex = *pp-'0';
						else hex = (*pp|0x20)-87;
						pp++;
						flag_last = S2H_HEX;
						continue;
					}else if(flag == S2H_SPACE){
						flag_last = S2H_SPACE;
						pp++;
						continue;;
					}else if(flag==S2H_END){
						goto _exit_for;
					}
				}
			}
		}
	_parse_error:
		if(hexarray != *ppBuffer){
			memory.free((void**)&hexarray,"<utils.str2hex>");
		}
		return 0|((unsigned int)pp-(unsigned int)str);
	_exit_for:
		//printf("解析了:%d\n",pba-(unsigned int)ba);
		*ppBuffer = hexarray;
		return count|0x80000000;
	}

	static __inline void hex2chs_append_nl(unsigned char** pp, c_text_formatting::newline_type nlt)
	{
		switch(nlt)
		{
		case c_text_formatting::newline_type::NLT_CR:
			**pp = '\r';
			++*pp;
			break;
		case c_text_formatting::newline_type::NLT_LF:
			**pp = '\n';
			++*pp;
			break;
		case c_text_formatting::newline_type::NLT_CRLF:
			**pp = '\r';
			++*pp;
			**pp = '\n';
			++*pp;
			break;
		}
	}

	char* c_text_formatting::hex2chs( unsigned char* hexarray,int length,char* buf,int buf_size, enum newline_type nlt )
	{
		char* buffer=NULL;
		unsigned char* p;
		int total_length; // 并非真正的总长度,依靠pch指针判断
		int cnt_n = 0;

		//计算以上各种情况出现的个数
		//其实按照最大来估计空间
		do{
			int i;
			for(i=0; i<length; i++){
				if(hexarray[i]=='\r' || hexarray[i]=='\n'){
					cnt_n++;
				}
			}
		}while(0);

		total_length = (length-cnt_n)*1 //非'\r','\n'
			+ cnt_n * (nlt == NLT_CRLF ? 2 : 1)
			+ 1;

		if(total_length<=buf_size && buf){
			buffer = buf;
		}else{
			buffer = (char*)GET_MEM(total_length);
			if(!buffer){
				if(buf)
					*buf = '\0';
				return buf;
			}
		}

		p = (unsigned char*)buffer;
		do{
			int i, step;
			for(i=0; i<length; i+=step+1){
				step=0;
				if(hexarray[i]=='\r'){
					hex2chs_append_nl(&p, nlt);
					if(i<length-1 && hexarray[i+1]=='\n'){
						step++;
						if( (i<length-2 && hexarray[i+2]=='\r') 
							&& ((i==length-3) || (i<length-3 && hexarray[i+3]!='\n')) )
						{
							step++;
						}
					}
				}
				else if(hexarray[i]=='\n'){
					hex2chs_append_nl(&p, nlt);
				}
				else if(hexarray[i]==0){
					// 如果需要"更形象"地显示'\0', 可以在这里处理
					// 并在前面加上对'\0'个数的计算
				}
				else{
					*p++ = hexarray[i];
				}
			}
		}while((0));

		*p++ = '\0';
		return buffer;
	}

	char* c_text_formatting::hex2str(unsigned char* hexarray, int* length, int linecch, int start, char* buf, int buf_size, enum newline_type nlt)
	{
		char* buffer = NULL;
		char* pb = NULL;
		int count = start;
		int total_length;
		int k;
		int nltsz = nlt == newline_type::NLT_CRLF ? 2 : 1;

		//2013-01-17更新计算错误:
		//	每字节占用2个ASCII+1个空格:length*3
		//  换行字符占用:length/linecch*nltsz
		if (linecch){
			total_length = *length * 3 + *length / linecch * nltsz + 1 + nltsz;//+1:最后1个'\0';+nltsz:可能是第1个\r\n
		}
		else{
			total_length = *length * 3 + 1 + nltsz;//+1:最后1个'\0';+2:可能是第1个\r\n
		}
		if (buf_size >= total_length && buf){
			buffer = buf;
		}
		else{
			buffer = (char*)GET_MEM(total_length);
			if (buffer == NULL) return NULL;
		}
		for (k = 0, pb = buffer; k < *length; k++){
			sprintf(pb, "%02X ", hexarray[k]);
			pb += 3;
			//换行处理
			if (linecch && ++count == linecch){
				hex2chs_append_nl((unsigned char**)&pb, nlt);
				count = 0;
			}
		}
		*pb = '\0';
		*length = pb - buffer;
		return buffer;
	}

	void set_clipboard_data(const char* str)
	{
		if (!str || !*str) return;

		if (OpenClipboard(NULL)){
			HGLOBAL hMem = NULL;
			int len;

			len = strlen(str) + 1;
			hMem = GlobalAlloc(GHND, len);
			if (hMem){
				char* pmem = (char*)GlobalLock(hMem);
				EmptyClipboard();
				memcpy(pmem, str, len);
				SetClipboardData(CF_TEXT, hMem);
			}
			CloseClipboard();
			if (hMem){
				GlobalFree(hMem);
			}
		}
	}

	void split_string(std::vector<std::string>* vec, const char* str, char delimiter)
	{
		const char* p = str;
		std::string tmp;
		for (;;){
			if (*p){
				if (*p != delimiter){
					tmp += *p;
					p++;
					continue;
				}
				else{
					vec->push_back(tmp);
					tmp = "";
					p++;
					continue;
				}
			}
			else{
				if (tmp.size()) vec->push_back(tmp);
				break;
			}
		}
	}
}

