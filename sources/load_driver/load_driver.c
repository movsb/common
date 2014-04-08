#include <windows.h>
#include <stdio.h>
#define LOAD_DRIVER
#include "load_driver.h"

static SC_HANDLE hScManager;

static void drvShowError(char* msg);
static int drvOpenScManager(int open);
static int drvCreateService(char* DriverAbsolutePath,char* ServiceName,char* ServiceDispalyName,SC_HANDLE* phService);
static int drvAddService(char* DriverAbsPath, char* ServiceName, char* DisplayName, int PromptIfExists);
static int drvDeleteService(char* ServiceName);

void init_driver(void)
{
	memset(&driver,0,sizeof(driver));
	driver.load = LoadDriver;
	driver.unload = UnloadDriver;
}

/**************************************************
函  数:drvShowError@4
功  能:显示最后一次系统函数调用的错误消息
参  数:msg - 消息前缀说明
返回值:(none)
说  明:内部调用,这里使用MessageBox,可以改成自己的
**************************************************/
void drvShowError(char* msg)
{
	void* pBuffer = NULL;
	DWORD dwLastError;
	dwLastError = GetLastError();
	if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&pBuffer, 1, NULL))
	{
		char buf[1024];
		_snprintf(buf,sizeof(buf),"%s\n\n%s", msg,pBuffer);
		MessageBox(NULL,buf,NULL,MB_OK);
		LocalFree((HLOCAL)pBuffer);
	}
}

/**************************************************
函  数:drvOpenScManager@4
功  能:打开服务控制管理器
参  数:open - !0:打开,0:关闭
返回值:成功:!0;失败:0
说  明:内部调用
	2013-02-17:
		增加了 引用计数以支持加载多个驱动,未测试
**************************************************/
int drvOpenScManager(int open)
{
	static DWORD refcount=0;
	if(open){
		if(hScManager){
			InterlockedIncrement(&refcount);
			return 1;
		}
		hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if(hScManager == NULL){
			drvShowError("打开服务控制管理器失败!");
			return 0;
		}
		InterlockedIncrement(&refcount);
		return 1;
	}else{
		if(hScManager&&!InterlockedDecrement(&refcount)){
			CloseServiceHandle(hScManager);
			hScManager=NULL;
		}
		return 1;
	}
}

/**************************************************
函  数:drvCreateService@16
功  能:创建新服务,并返回服务句柄(if succeeded)
参  数:	DriverAbsolutePath - 驱动文件的绝对路径
		ServiceName - 服务名
		ServiceDisplayName - 服务的显示名
		*phService - 返回的服务句柄
返回值:成功:!0;失败:0
说  明:内部调用
**************************************************/
int drvCreateService(char* DriverAbsolutePath,char* ServiceName,char* ServiceDispalyName,SC_HANDLE* phService)
{
	SC_HANDLE hService;
	hService = CreateService(
		hScManager,				//服务控制器管理器句柄
		ServiceName,			//服务的名称
		ServiceDispalyName,		//服务的显示名称
		SERVICE_ALL_ACCESS,		//对该服务的访问权限
		SERVICE_KERNEL_DRIVER,	//服务的类型:内核驱动
		SERVICE_DEMAND_START,	//启动类型:手动启动
		SERVICE_ERROR_NORMAL,	//服务错误控制:正常
		DriverAbsolutePath,		//服务文件的绝对路径
		NULL,					//没有启动组
		NULL,					//不更改默认的标签ID
		NULL,					//没有服务依赖项
		NULL,					//使用默认对象名称
		NULL					//没有密码
		);
	*phService = hService;
	return hService!=NULL;
}

/**************************************************
函  数:drvDeleteService@4
功  能:删除指定服务名的的服务
参  数:ServiceName - 服务名
返回值:成功:!0;失败:0
说  明:	内部调用
		对不存在的服务返回-1(成功)
**************************************************/
int drvDeleteService(char* ServiceName)
{
	int sehcode=0;
	SERVICE_STATUS ServiceStatus;
	SC_HANDLE hService=NULL;
	DWORD dwLastError;
	__try{
		hService=OpenService(hScManager,ServiceName,SERVICE_ALL_ACCESS);
		if(hService==NULL){
			dwLastError=GetLastError();
			if(dwLastError==ERROR_SERVICE_DOES_NOT_EXIST){
				sehcode=-1;
				__leave;
			}else{
				drvShowError("在打开已经存在的服务时遇到以下错误:");
				__leave;
			}
		}
		if(!ControlService(hService,SERVICE_CONTROL_STOP,&ServiceStatus)){//停止控制失败
			dwLastError = GetLastError();
			if(dwLastError != ERROR_SERVICE_NOT_ACTIVE){//并不是因为没有启动而出错
				drvShowError("在停止服务时遇到以下错误:");
				__leave;
			}
		}
		if(!DeleteService(hService)){
			drvShowError("在删除已存在的服务时遇到以下错误:");
			__leave;
		}
		sehcode=1;
	}
	__finally{
		if(hService){
			CloseServiceHandle(hService);
			hService=NULL;
		}
	}
	return sehcode;
}

/**************************************************
函  数:drvAddService@12
功  能:添加指定的服务
参  数:	DriverAbsPath - 驱动程序绝对路径
		ServiceName - 服务名
		DisplayName - 服务显示名
		PromptIfExists - 存在的时候是否继续:
			1:删除并重新创建
			0:不再继续,返回-1(成功)
			-1:提示是否继续
返回值:成功:!0;失败:0
说  明:	内部调用
		若选择了不再继续,返回-1(成功)
**************************************************/
int drvAddService(char* DriverAbsPath, char* ServiceName, char* DisplayName, int PromptIfExists)
{
	int sehcode=0;
	SC_HANDLE hService = NULL;		//创建/打开的服务句柄
	DWORD dwErrCode = 0;
	__try{
		//假定服务不存在并创建
		if(!drvCreateService(DriverAbsPath,ServiceName,DisplayName,&hService)){
			//服务创建失败,可能已经存在
			DWORD dwLastError = GetLastError();
			//如果是服务已经存在,删除,重新安装
			if(dwLastError == ERROR_SERVICE_EXISTS){
				switch(PromptIfExists)
				{
				case 1:break;//重新创建
				case 0:{sehcode=-1;__leave;break;}
				case -1:
					{
						char* yesmsg = "指定的服务已经存在,要继续创建吗?";
						if(MessageBox(NULL,yesmsg,DisplayName,MB_ICONQUESTION|MB_YESNO)!=IDYES){
							sehcode=-1;
							__leave;
						}
						break;
					}
				}
				if(!drvDeleteService(ServiceName)){
					__leave;
				}
				if(!drvCreateService(DriverAbsPath,ServiceName,DisplayName,&hService)){
					drvShowError("重新创建服务时遇到以下错误:");
					__leave;
				}
			}else{//其它原因造成服务创建失败
				drvShowError("创建服务时遇到以下错误:");
				__leave;
			}
		}
		//服务成功创建来到这里
		if(!StartService(hService,0,NULL)){
			drvShowError("在启动服务时遇到以下错误:");
			__leave;
		}
		sehcode=1;
	}
	__finally{
		if(hService){
			CloseServiceHandle(hService);
			hService=NULL;
		}
	}
	return sehcode;
}

/**************************************************
函  数:LoadDriver@12
功  能:加载指定驱动
参  数:	DriverAbsPath - 驱动程序绝对路径
		ServiceName - 服务名
		DisplayName - 服务显示名
		PromptIfExists - 存在的时候是否继续:
			1:删除并重新创建
			0:不再继续,返回-1(成功)
			-1:提示是否继续
返回值:成功:!0;失败:0
说  明:	外部函数
		加载失败并不一定是完全失败,所有请始终成
对地调用LoadDriver/UnloadDriver以清除注册表残留信息
**************************************************/
int LoadDriver(char* DriverAbsPath, char* ServiceName, char* DisplayName,int PromptIfExists)
{
	if(!drvOpenScManager(1))
		return 0;
	return drvAddService(DriverAbsPath,ServiceName,DisplayName, PromptIfExists);
}

/**************************************************
函  数:UnloadDriver@4
功  能:卸载指定名称的驱动服务
参  数:ServiceName - 服务的名称
返回值:成功:!0;失败:0
说  明:	外部函数
		对不存在的服务返回-1(成功)
**************************************************/
int UnloadDriver(char* ServiceName)
{
	int ret;
	ret=drvDeleteService(ServiceName);
	drvOpenScManager(0);
	return ret;
}

//~~~示例~~~
#if 0
int main(void){
	int err;
	char sys[MAX_PATH];
	GetModuleFileName(NULL,sys,sizeof(sys));
	strcpy(strrchr(sys,'\\'),"\\drv.sys");
	printf("sys:%s\n",sys);
	err=LoadDriver(sys, "drv","drv Test Service",-1);
	MessageBox(NULL,err==-1?"驱动已存在!":err?"驱动已加载!":"驱动未能成功加载!","",MB_OK);
	if(err!=-1){
		err=UnloadDriver("drv");
		MessageBox(NULL,err==-1?"驱动不存在!":err?"驱动已卸载!":"驱动未能成功卸载!","",MB_OK);
	}
	return 0;
}
#endif
