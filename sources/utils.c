#define __UTILS_C__
#include "utils.h"
#include "msg.h"
#include "expr.h"
#include "about.h"
#include "debug.h"
#include "struct/memory.h"
#include "comm.h"
#include "../res/resource.h"

struct utils_s utils;

static char* __THIS_FILE__ = __FILE__;

void init_utils(void)
{
	memset(&utils, 0, sizeof(utils));
	utils.msgbox                   = msgbox;
	utils.msgerr                   = msgerr;
	utils.get_file_name            = get_file_name;
	utils.set_clip_data            = set_clip_data;
	utils.str2hex                  = str2hex;
	utils.hex2str                  = hex2str;
	utils.center_window            = center_window;
	utils.show_expr                = ShowExpr;
	utils.hex2chs                  = hex2chs;
	utils.assert_expr              = myassert;
	utils.wstr2lstr                = wstr2lstr;
	utils.check_chs                = check_chs;
	utils.remove_string_return     = remove_string_return;
	utils.remove_string_linefeed   = remove_string_linefeed;
	utils.parse_string_escape_char = parse_string_escape_char;
	utils.eliminate_control_char   = eliminate_control_char;
	return;
}

/***************************************************
函  数:msgbox
功  能:显示消息框
参  数:
	msgicon:消息光标
	caption:对话框标题
	fmt:格式字符串
	...:变参
返回值:
	用户点击的按钮对应的值(MessageBox)
说  明:
***************************************************/
int msgbox(HWND hOwner,UINT msgicon, char* caption, char* fmt, ...)
{
	va_list va;
	char smsg[1024]={0};
	va_start(va, fmt);
	_vsnprintf(smsg, sizeof(smsg), fmt, va);
	va_end(va);
	return MessageBox(hOwner, smsg, caption, msgicon);
}
/***************************************************
函  数:msgerr
功  能:显示带prefix前缀的系统错误消息
参  数:prefix-前缀字符串
返回值:(无)
说  明:
***************************************************/
void msgerr(HWND hOwner,char* prefix)
{
	char* buffer = NULL;
	if(!prefix) prefix = "";
	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),(LPSTR)&buffer,1,NULL)) 
	{
		utils.msgbox(hOwner,MB_ICONHAND, NULL, "%s:%s", prefix, buffer);
		LocalFree(buffer);
	}
}

/***************************************************
函  数:get_file_name
功  能:显示打开/保存文件对话框
参  数:
	title:对话框标题
	filter:对话框过虑选项
	action:打开的动作: 0-打开,1-保存
	opentype:0-16进制,1-文本,2-命令(仅打开时),用来返回
返回值:
	NULL:没有正确地选择
	否则:选择的文件名字符串
说  明:我去,从一开始到现在(2013-02-06),才找到那个无法显示
	OFN_EXPLORER风格的原因,就一个_WIN32_WINNT啊,啊啊啊~
***************************************************/
UINT_PTR __stdcall OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
char* get_file_name(char* title, char* filter, int action, int* opentype)
{
	OPENFILENAME ofn = {0};
	int ret;
	static char buffer[MAX_PATH];
	*buffer = 0;
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = msg.hInstance;
	if(action == 0) ofn.Flags = OFN_ENABLESIZING|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE;
	else ofn.Flags = OFN_PATHMUSTEXIST|OFN_ENABLESIZING|OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	ofn.hwndOwner = msg.hWndMain;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = &buffer[0];
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = title;
	ofn.lpfnHook = OFNHookProc;
	ofn.lCustData = (LPARAM)opentype;
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DLG_TEMPLATE);
	ret = action?GetSaveFileName(&ofn):GetOpenFileName(&ofn);
	return ret?&buffer[0]:NULL;
}

UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hParent = NULL;
	static int* popentype = NULL;
	if(uiMsg == WM_NOTIFY){
		LPOFNOTIFY pofn = (LPOFNOTIFY)lParam;
		if(pofn->hdr.code == CDN_FILEOK){
			HWND hCboOpenType = GetDlgItem(hdlg, IDC_HOOK_COMBO);
			int index = ComboBox_GetCurSel(hCboOpenType);
			if(index == -1){
				int id;
				id=MessageBox(GetParent(hdlg), 
					"请在下面的列表中选择文件打开/保存的方式!\n\n"
					"如果你不清楚这两种方式的区别,请点击\"确定(OK)\"查看简要的帮助!", 
					"未选择文件操作方式", MB_ICONEXCLAMATION|MB_OKCANCEL);
				if(id==IDOK){
					//todo:增加命令说明....
					char* helpmsg = 
						"(1)对于加载文件\n"
						"    (a)选择十六进制\n"
						"        该文件为任意格式的文件,程序会将该文件的内容解析为16进制序列的文本字符串\n"
						"    (b)选择文本字符\n"
						"        该文件包含有效的16进制序列文本,有效的16进制序列文本为每个字节用两个16进制字符指示的文本,程序直接把它放到发送缓冲区中,只进行语法检查\n"
						"\n"
						"(2)对于保存文件:\n"
						"    (a)选择十六进制\n"
						"        <1>如果以16进制方式显示:程序会把接收数据区中的文本按每两个字符为1组保存到16进制文件,而不是直接保存16进制序列内容,如果您接收的数据是ASCII码/有效中文,则保存后,就可以直接打开并显示出来了\n"
						"        <2>如果以字符方式显示:不允许保存为16进制内容,因为格式不符合要求\n"
						"    (b)选择文本字符\n"
						"        <1>如果以16进制方式显示:程序会直接把16进制序列文本保存到指定的文件,这和接收数据区中显示的一样\n"
						"        <2>如果以文本字符方式显示:程序会直接保存文本到指定文件,同上面的16进制一样";
					MessageBox(GetParent(hdlg),helpmsg,"关于保存/打开方式",MB_OK);
				}
				//2013-01-17
				SetFocus(hCboOpenType);
				SetWindowLong(hdlg, DWL_MSGRESULT, 1);
				return 1;
			}
			*popentype = index;
			return 0;
		}
	}else if(uiMsg == WM_SIZE){
		HWND hCboCurFlt = GetDlgItem(GetParent(hdlg), cmb1);
		HWND hCboOpenType = GetDlgItem(hdlg, IDC_HOOK_COMBO);
		RECT rcCboFlt;
		GetWindowRect(hCboCurFlt, &rcCboFlt);
		SetWindowPos(hCboOpenType,HWND_NOTOPMOST,0,0,rcCboFlt.right-rcCboFlt.left,rcCboFlt.bottom-rcCboFlt.top, SWP_NOMOVE|SWP_NOZORDER);
		return 0;
	}else if(uiMsg == WM_INITDIALOG){
		HWND hCboOpenType = GetDlgItem(hdlg, IDC_HOOK_COMBO);
		ComboBox_AddString(hCboOpenType, "数据文件, 任意类型(包含普通文本文件)");
		ComboBox_AddString(hCboOpenType, "十六进制, 包含16进制序列的文本文件");
		//todo:仅打开时才存在
		ComboBox_AddString(hCboOpenType, "命令文件, 包含命令列表的文本文件");
		
		popentype = (int*)((OPENFILENAME*)lParam)->lCustData;
		return 0;
	}
	UNREFERENCED_PARAMETER(wParam);
	return 0;
}

/***************************************************
函  数:set_clip_data
功  能:复制str指向的字符串到剪贴板
参  数:
	str:字符串,以0结尾
返回值:
	成功:非零
	失败:零
说  明:
***************************************************/
int set_clip_data(char* str)
{
	HGLOBAL hGlobalMem = NULL;
	char* pMem = NULL;
	int lenstr;

	if(str == NULL) return 1;
	if(!OpenClipboard(NULL)) return 0;

	lenstr = strlen(str)+1;//Makes it null-terminated
	hGlobalMem = GlobalAlloc(GHND, lenstr);
	if(!hGlobalMem) return 0;
	pMem = (char*)GlobalLock(hGlobalMem);
	EmptyClipboard();
	memcpy(pMem, str, lenstr);
	SetClipboardData(CF_TEXT, hGlobalMem);
	CloseClipboard();
	GlobalFree(hGlobalMem);
	return 1;
}

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
enum{S2H_NULL,S2H_SPACE,S2H_HEX,S2H_END};

unsigned int str2hex(char* str, unsigned char** ppBuffer,unsigned int buf_size)
{
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
		memory.free_mem((void**)&hexarray,"<utils.str2hex>");
	}
	return 0|((unsigned int)pp-(unsigned int)str);
_exit_for:
	//printf("解析了:%d\n",pba-(unsigned int)ba);
	*ppBuffer = hexarray;
	return count|0x80000000;
}

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
	今天又遇到奇葩, 只使用\r换行, 你TM能再标准一点么!!!
		现在做如下统一换行要求:
			① \r 后面没有 \n
			② \n 
			③ \r\n
			④ \r\n\r
			如果出现上面四种情况, 均作为一个换行符处理
	突然发现, 其实在这里可以全部处理掉所有(最后一个除外)的'\0'.
**************************************************/
char* hex2chs(unsigned char* hexarray,int length,char* buf,int buf_size)
{
	char* buffer=NULL;
	int total_length;
	int line_r=0;
	int line_rnr=0;
	int z_cnt=0;
	int len_addend=0;
	//计算以上前两种情况出现的个数(第3种不需要处理)
	do{
		int len=0;
		for(line_r=0,line_rnr=0; len<length; len+=len_addend+1){
			len_addend=0;
			if(hexarray[len]=='\r'){
				if(len<length-1){
					if(hexarray[len+1]!='\n'){
						line_r++;
					}
					else{
						len_addend++; // skip next '\n'
						if(len<length-2){
							if(hexarray[len+2]=='\r'){
								if(len<length-3){
									if(hexarray[len+3]!='\n'){
										line_rnr++;
										len_addend ++; //skip next '\r'
									}
								}
								else{
									line_rnr++;
									len_addend ++;
								}
							}
						}
					}
				}
				else{
					line_r++;
				}
			}
			else if(hexarray[len]=='\n'){
				line_r++;
			}
			else if(hexarray[len]==0){
				z_cnt++;
			}
		}
	}while((0));

	total_length = 0
		+ length*1 - line_r*1 - line_rnr*3	// 其中 前两种情况 已经在计算转换为'\r\n'时计算过
		+ line_r * 2						// 每个 前两种情况之一 会被转换成 '\r\n'
		+ line_rnr * 2						// 每个 '\r\n\r' 换成 '\r\n'
		- z_cnt								// 0 不需要保存下来
		+ 1									// 以'\0'结尾
		;
	if(total_length<=buf_size && buf){
		buffer = buf;
		//memset(buffer,0,buf_size);
	}else{
		buffer = (char*)GET_MEM(total_length);
		if(!buffer) return NULL;
	}

	// 转换前两种情况 + 处理'\0'
	do{
		unsigned char* pch=(unsigned char*)buffer;
		int itx,itx_addend;
		for(itx=0; itx<length; itx+=itx_addend+1){
			itx_addend=0;
			if(hexarray[itx]=='\r'){
				*pch++ = '\r';
				*pch++ = '\n';

				if(itx<length-1){
					if(hexarray[itx+1]=='\n'){
						itx_addend++;
						if(itx<length-2){
							if(hexarray[itx+2]=='\r'){
								if(itx<length-3){
									if(hexarray[itx+3]!='\n'){
										itx_addend++;
									}
								}
							}
						}
					}
				}
			}
			else if(hexarray[itx]=='\n'){
				*pch++ = '\r';
				*pch++ = '\n';
			}
			else if(hexarray[itx]==0){

			}
			else{
				*pch++ = hexarray[itx];
			}
		}
// 		if((((unsigned int)pch-(unsigned int)buffer) != total_length-1))
// 		{
// 			utils.assert_expr(NULL,"");
// 		}
	}while((0));
	buffer[total_length-1] = '\0';
	return buffer;
}

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
char* hex2str(unsigned char* hexarray, int* length, int linecch, int start, char* buf, int buf_size)
{
	char* buffer = NULL;
	char* pb = NULL;
	int count = start;
	int total_length;
	//int ret_length=0;
	int k;

	//2013-01-17更新计算错误:
	//	每字节占用2个ASCII+1个空格:length*3
	//  换行字符占用:length/linecch*2
	if(linecch){
		total_length = *length*3 + *length/linecch*2+1+2;//+1:最后1个'\0';+2:可能是第1个\r\n
	}else{
		total_length = *length*3+1+2;//+1:最后1个'\0';+2:可能是第1个\r\n
	}
	if(buf_size>=total_length && buf){
		buffer = buf;
		//memset(buffer,0,buf_size);
	}else{
		buffer=(char*)GET_MEM(total_length);
		if(buffer == NULL) return NULL;
	}
	//memset(buffer,0,total_length);
	for(k=0,pb=buffer; k<*length; k++){
		sprintf(pb, "%02X ", hexarray[k]);
		pb += 3;
		//换行处理
		if(linecch && ++count == linecch){
			pb[0] = '\r';
			pb[1] = '\n';
			pb += 2;
			count = 0;
		}
	}
	//2013-03-10:以前少加了一句:*pb='\0'; 
	//导致接收区总是显示乱码(数据量不正确),找了好久才发现....
	*pb = '\0';
	*length = pb-buffer;
	return buffer;
}

/**************************************************
函  数:center_window@8
功  能:把指定窗口居中于指定的另一窗口
参  数:	hWnd - 待居中的窗口句柄
		hWndOwner - 参考窗口句柄
返回值:
说  明:若居中于屏幕,置hWndOwner为NULL
**************************************************/
void center_window(HWND hWnd, HWND hWndOwner)
{
	RECT rchWnd,rchWndOwner;
	int width,height;
	int x,y;

	if(!IsWindow(hWnd)) return;
	GetWindowRect(hWnd,&rchWnd);

	if(!hWndOwner||!IsWindow(hWndOwner)){
		int scrWidth,scrHeight;
		scrWidth = GetSystemMetrics(SM_CXSCREEN);
		scrHeight = GetSystemMetrics(SM_CYSCREEN);
		SetRect(&rchWndOwner,0,0,scrWidth,scrHeight);
	}else{
		GetWindowRect(hWndOwner,&rchWndOwner);
	}
	width = rchWnd.right-rchWnd.left;
	height = rchWnd.bottom-rchWnd.top;
	
	x = (rchWndOwner.right-rchWndOwner.left-width)/2+rchWndOwner.left;
	y = (rchWndOwner.bottom-rchWndOwner.top-height)/2+rchWndOwner.top;

	MoveWindow(hWnd,x,y,width,height,TRUE);
}

/***********************************************************************
名称:Assert
描述:Debug
参数:pv-任何表达式,str-提示
返回:
说明:
***********************************************************************/
void myassert(void* pv,char* str)
{
	if(!pv){
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"Debug Error:%s\n\n"
			"应用程序遇到内部错误,请报告错误!\n\n"
			"请重新启动应用程序后再试!",str);
	}
}

/***********************************************************************
名称:wstr2lstr (Windows -> Linux (Standard new line format))
描述:转换'\r\n'回车换行格式的字符串到'\n'格式
参数:src-待转换的字符串
返回:0-失败,其它:转换后的长度,包含'\0'
说明:原字符串被修改,src必须指向可修改内存
***********************************************************************/
int wstr2lstr(char* src)
{
	char* pdst = src;
	char* psrc = src;
	if(pdst == NULL){
		debug_out(("utils.wstr2lstr:src == NULL!(file:%s,line:%d)\n",__FILE__,__LINE__));
		return 0;
	}
	for(;;){
		if(*psrc=='\r' && *(psrc+1) && *(psrc+1)=='\n'){
			*pdst++ = '\n';
			psrc += 2;
		}else{
			*pdst++ = *psrc;
			if(!*psrc){
				break;
			}else{
				psrc++;
			}
		}
	}
	return (unsigned int)(pdst-(unsigned int)src);
}

/***********************************************************************
名称:check_chs
描述:检测ba数组中的数据是否为合法的中英文序列,中文为两个>=0x80的值,英文小于0x80
参数:ba-数组,cb字节数
返回:!0-最后需要一个中文字符,0-合法
说明:如果检测到一个>0x7F+一个小于等于0x7F,则大于的会被改成'?'
***********************************************************************/
int check_chs(unsigned char* ba, int cb)
{
	int it;
	enum{
		CHARFMT_NULL,
		CHARFMT_ASCII,
		CHARFMT_OEMCP
	};
	int flag=CHARFMT_NULL;
	int flag_current=CHARFMT_NULL;
	for(it=0; it<cb;it++){
		flag_current = ba[it]<=0x7F?CHARFMT_ASCII:CHARFMT_OEMCP;
		switch(flag)
		{
		case CHARFMT_NULL:
			flag = flag_current;
			break;
		case CHARFMT_ASCII:
			if(flag_current == CHARFMT_ASCII){
				continue;
			}else if(flag_current == CHARFMT_OEMCP){
				flag = CHARFMT_OEMCP;
			}
			break;
		case CHARFMT_OEMCP:
			if(flag_current == CHARFMT_ASCII){
				ba[it-1] = '?';
				flag = CHARFMT_ASCII;
			}else if(flag_current == CHARFMT_OEMCP){
				flag = CHARFMT_NULL;
			}
			break;
		}
	}
	return flag_current==CHARFMT_OEMCP && flag==CHARFMT_OEMCP;
}

/**************************************************
函  数:remove_string_return
功  能:移除字符串中的 回车换行
参  数:str - 待剔除回车换行的字符串
返  回:结果字符串的长度
说  明: '\r','\n' 均被剔除
**************************************************/
unsigned int remove_string_return(char* str)
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

/**************************************************
函  数:remove_string_linefeed
功  能:移除字符串中的 '\r'
参  数:str - 待剔除回车换行的字符串
返  回:结果字符串的长度
说  明: 
**************************************************/
unsigned int remove_string_linefeed(char* str)
{
	char* p1 = str;
	char* p2 = str;

	while(*p2){
		if(*p2=='\r'){
			p2++;
		}else{
			*p1++ = *p2++;
		}
	}
	*p1 = '\0';
	return (unsigned int)p1-(unsigned int)str;
}

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
**************************************************/
unsigned int parse_string_escape_char(char* str)
{
	char* p1 = str;
	char* p2 = str;

	while(*p2){
		if(*p2 == '\\'){
			p2++;
			switch(*p2)
			{
			case '\\':*p1++ = '\\';p2++;break;
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
							unsigned char hex =*p2-'0';
							hex = (hex<<4) + (*(p2+1)-'0');
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
				goto _error;
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

/**************************************************
函  数: eliminate_control_char
功  能: 处理字符串str中的控制字符, 当前只处理\b删除字符
参  数: str - 待处理的字符串
返  回: 返回需要额外删除的字符的个数
说  明: 由于可能多个\b连在一起, 所以应该根据返回值来确定需要向前删除
	多少个字符; \b只可能出现在所有的非\b之前;
	比如: "\b\bABC\b" 函数返回2
	可以像下面这样使用此函数:
	char str[] = "xxx";
	unsigned int i = eliminate_control_char(str);
	char* p = str;
	do{
		if(*p != '\b'){
			追加输出;
			//一定会在这里退出循环,无需修改p
		}
		else{
			向前删除一个已经存在的字符
			p++;
		}
	while(i--);
**************************************************/
unsigned int eliminate_control_char(char* str)
{
	unsigned int unhandled = 0;
	char* p = str;
	char* q = str+1;

	if(!p || !*p)
		return unhandled;

	while(*p){
		if(*p=='\b'){
			unhandled++;
			break;
		}
		p++;
	}
	if(!unhandled)
		return 0;

	unhandled = 0;
	p = str;

	if(*p=='\b') 
		unhandled++;

	while(*q){
		if(*q != '\b'){
			*++p = *q++;
		}
		else{
			while(*q && *q=='\b'){
				if(p>=str && *p!='\b'){
					--p;
					++q;
				}
				else{
					unhandled++;
					*++p = *q++;
				}
			}
		}
	}
	*++p = '\0';
	return unhandled;
}
