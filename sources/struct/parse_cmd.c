#include "../debug.h"
#include "../utils.h"
#include "memory.h"
#include "parse_cmd.h"

/**********************************************************
文件名称:parse_cmd.c/parse_cmd.h
文件路径:./common/
创建时间:2013-07-26 21:20
文件作者:女孩不哭
文件说明:该文件函数解析待发送的数据的命令文件,使用方法见文件末尾的main函数
	注意:
		1.如果是字符串格式, 默认开启转义字符功能
		2.所有数据必需写在一行, 不能换行,要换行请使用 '\n'

	命令文件格式:
		方式:名字:命令数据(需写在同一行)
	方式:
		命令数据为16进制数组,如:00 12 34 56
		命令数据为字符内容,如:abcdefg\r\n12345
	名字:
		当前命令的名字
	命令数据:
		16进制数组和字符序列

	方式,名字,命令数据的最大长度见宏定义
**********************************************************/

static char* __THIS_FILE__ = __FILE__;

#define FREE_MEM(p,s) memory.free_mem((void**)(p),s)

int read_file_content(char* file,char** pbuf,size_t* size)
{
	FILE* fp = NULL;
	size_t file_size = 0;
	char* buffer = NULL;

	fp = fopen(file,"rb");	//Must be "rb", never use "rt"
	if(fp == NULL){
		return 0;
	}
	fseek(fp,0,SEEK_END);
	file_size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	
	//由于是文本文件,多一个字节以保证最后一个字节为'\0'
	file_size++;
	buffer = (char*)GET_MEM(file_size);
	if(buffer == NULL){
		fclose(fp);
		return 0;
	}
	memset(buffer,0,file_size);
	fread(buffer,1,file_size-1,fp);
	fclose(fp);
	*pbuf = buffer;
	*size = file_size-1;/////2013年11月2日 18:38:16 BUGBUGBUGBUG  MBMB :((( file_size
	return 1;
}



#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a)))


int parse_command_list(char* buffer,size_t size,command_item** ppci,size_t* nitems)
{
	char* ptr = NULL;
	char ch;

	enum{
		PARSE_INVALID,
		PARSE_METHOD,
		PARSE_NAME,
		PARSE_COMMAND,
		PARSE_COMMENT,
	};
	//int flag = PARSE_METHOD;
	int flag_last = PARSE_METHOD;
	int it=0;

	int it_of_item = 0;
	//command_item* *ppci = (command_item**) .... 下面当作结构体使用只分配一次内存
	command_item* pci = (command_item*)GET_MEM(sizeof(command_item)*CMDI_MAX_LINES);
	if(pci == NULL){
		return 0;
	}

	*ppci = NULL;
	*nitems = 0;

	memset(pci,0,sizeof(command_item)*CMDI_MAX_LINES);
	for(ptr=buffer;;){
		ch = *ptr;
		switch(flag_last)
		{
		case PARSE_METHOD:
			{
				if(it_of_item >= CMDI_MAX_LINES){
					goto _exit_for;
				}
				switch(ch)
				{
				case 'C':
				case 'H':
					if(it < ARRAY_SIZE(pci[0].method)-1){
						pci[it_of_item].method[it++] = ch;
					}
					ptr++;
					break;
				case '#':
					if(it == 0){
						flag_last = PARSE_COMMENT;
					}
					ptr++;
					break;
				case '\r':
				case '\n':
					ptr++;
					if(*ptr=='\r' || *ptr=='\n'){
						ptr++;
					}
					it = 0;
					flag_last = PARSE_METHOD;
					break;
				case '\0':
					goto _exit_for;
				case ':':
					if(it <= ARRAY_SIZE(pci[0].method)-1){
						pci[it_of_item].method[it++] = 0;
					}
					flag_last = PARSE_NAME;
					it = 0;
					ptr++;
					break;
				default:
					if(it < ARRAY_SIZE(pci[0].method)-1){
						pci[it_of_item].method[it++] = ch;
					}else if(it == ARRAY_SIZE(pci[0].method)-1){
						pci[it_of_item].method[it++] = 0;
					}
					ptr++;
					break;
				}
				continue;
			}
		case PARSE_NAME:
			{
				switch(ch)
				{
				case ':':
					if(it <= ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = 0;
					}
					flag_last = PARSE_COMMAND;
					ptr++;
					it = 0;
					break;
				case '\r':
				case '\n':
					ptr++;
					if(*ptr=='\r' || *ptr=='\n'){
						ptr++;
					}
					if(it <= ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = 0;
					}
					it = 0;
					flag_last = PARSE_METHOD;
					break;
				case '\0':
					if(it <= ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = 0;
					}
					it_of_item++;
					goto _exit_for;
					break;
				default:
					if(it < ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = ch;
					}else if(it == ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = 0;
					}
					ptr++;
					break;
				}
				continue;
			}
		case PARSE_COMMAND:
			{
				if(ch=='\r' || ch=='\n'){
					ptr++;
					if(ptr[0] == '\r' || ptr[0] == '\n'){
						ptr++;
					}
					if(it <= ARRAY_SIZE(pci[0].command)-1){
						pci[it_of_item].command[it++] = 0;
					}
					it = 0;
					it_of_item++;
					flag_last = PARSE_METHOD;
					continue;
				}else if(ch==0){
					if(it <= ARRAY_SIZE(pci[0].command)-1){
						pci[it_of_item].command[it++] = 0;
					}
					it_of_item++;
					goto _exit_for;
				}else{
					if(it < ARRAY_SIZE(pci[0].command)-1){
						pci[it_of_item].command[it++] = ch;
					}else if(it == ARRAY_SIZE(pci[0].command)-1){
						pci[it_of_item].command[it++] = 0;
					}
					ptr++;
					continue;
				}
			}
		case PARSE_COMMENT:
		case PARSE_INVALID:
			{
				if(ch=='\r' || ch=='\n'){
					ptr++;
					if(ptr[0]=='\r' || ptr[0]=='\n'){
						ptr++;
					}
					flag_last = PARSE_METHOD;
					continue;
				}else if(ch==0){
					goto _exit_for;
				}else{
					ptr++;
					continue;
				}
			}
		}//switch
	}//for
_exit_for:
	if(it_of_item < CMDI_MAX_LINES){
		//re-alloc
	}
	//todo:
	{//////////////////////////////////////////////////////////////////////////
		//2013年11月2日 14:35:20
		//由于考虑到用户可能会在编辑框中临时修改命令数据, 这里检查有效性也没多大意思, 注释掉部分代码~
		// 

// 		int x;
// 		for(x=0; x<it_of_item; x++){
// 			make_command_item(pci+x);
// 		}

	}//////////////////////////////////////////////////////////////////////////
	*ppci = pci;
	*nitems = it_of_item;
	return 1;
}

int make_command_item(command_item* pci)
{
	int ret;
	unsigned char hexa[CMDI_MAX_COMMAND];//tmp hex array
	unsigned char* hex_ptr = hexa;

	if(pci->method[0] == 'H'){//16
		ret = utils.str2hex((char*)pci->command,&hex_ptr,sizeof(hexa));
		if(ret & 0x80000000){
			size_t len = ret & 0x7FFFFFFF;
			pci->valid = 1;
			pci->bytes = len;
			//memcpy(pci->command,hex_ptr,len);
			memcpy(pci->data,hex_ptr,len);

			if(hex_ptr != hexa){
				memory.free_mem((void**)&hex_ptr,"make_command_item");
			}
		}else{
			pci->valid = 0;
			utils.msgbox(NULL,MB_ICONERROR,NULL,"命令 %s 的命令数据不是正确的16进制序列!",pci->name);
		}

	}else if(pci->method[0] == 'C'){//char
		unsigned int ret;
		int len;

		memcpy(pci->data,pci->command,sizeof(pci->command));

		ret = utils.parse_string_escape_char((char*)pci->data);
			
		len = ret & 0x7FFFFFFF;

		if(ret & 0x80000000){
			pci->bytes = len;//bytes的值为有效时的值, 若无效则无效, 根据valid判断
			pci->valid = 1;
		}else{
			//pci->bytes = strlen((char*)pci->command);
			utils.msgbox(NULL,MB_ICONEXCLAMATION, NULL, "解析命令 %s 时出错, 此命令将无效!\n\n"
				"在第 %d 个字符附近出现语法解析错误!",pci->name,len);
			pci->valid = 0;
		}
	}else{
		pci->valid = 0;
		utils.msgbox(NULL,MB_ICONERROR,NULL,"命令 %s 的数据类型不明确!",pci->name);
	}
	return pci->valid != 0;
}

int parse_cmd(char* file,command_item** ppci,size_t* pnItems)
{
	char* buf = NULL;
	size_t size = 0;
	if(read_file_content(file,&buf,&size)){
		parse_command_list(buf,size,ppci,pnItems);
		FREE_MEM(&buf,"");
		return 1;
	}else{
		*ppci = NULL;
		return 0;
	}
}

#if 0
int main(void)
{
	command_item* pci = NULL;
	size_t size = 0;
	size_t i;
	if(parse_cmd("command.txt",&pci,&size)){
		for(i=0; i<size; i++){
			printf("method:%c\nname:%s\ncommand:%s\n\n",pci[i].method[0],pci[i].name,pci[i].command);
		}
		FREE_MEM(&pci,"");
	}
	return 0;
}
#endif
