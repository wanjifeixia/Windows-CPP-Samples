//进程信息采集

#pragma once
#include <Windows.h>

//枚举进程
typedef BOOL(_stdcall* ENUMPROCESS)(
	DWORD* pProcessIds,    //指向进程ID数组链
	DWORD cb,              //ID数组的大小，用字节计数
	DWORD* pBytesReturned  //返回的字节
	);

//枚举进程模块
typedef BOOL(_stdcall* ENUMPROCESSMODULES)(
	HANDLE  hProcess,   //进程句柄
	HMODULE* lphModule, //指向模块句柄数组链
	DWORD   cb,         //模块句柄数组大小，字节计数
	LPDWORD lpcbNeeded  //存储所有模块句柄所需的字节数
	);

//获得进程模块名
typedef DWORD(_stdcall* GETMODULEFILENAMEEX)(
	HANDLE  hProcess,   //进程句柄
	HMODULE hModule,    //进程句柄
	LPTSTR  lpFilename, //存放模块全路径名
	DWORD   nSize       //lpFilename缓冲区大小，字符计算
	);

//获得进程名
typedef DWORD(_stdcall* GETMODULEBASENAME)(
	HANDLE  hProcess,  //进程句柄
	HMODULE hModule,   //模块句柄
	LPTSTR  lpBaseName,//存放进程名
	DWORD   nSize      //lpBaseName缓冲区大小
	);

//进程信息结构
typedef struct tagProcessInfo
{
	DWORD dwPID;//进程ID
	char  szFileName[MAX_PATH];//进程文件名
	char  szPathName[MAX_PATH];//进程路径名
}ProcessInfo;

class ProcessInfoCollect
{
public:
	ProcessInfoCollect();
	virtual ~ProcessInfoCollect();

	//获取当前进程的cpu使用率
	BOOL    GetCPUUserRate(double& dCPUUserRate);
	//获取指定进程的cpu使用率
	BOOL    GetCPUUserRate(DWORD lProcessID, double& dCPUUserRate);

	//获取当前进程的IO计数
	int		GetIOBytes(ULONGLONG* read_bytes, ULONGLONG* write_bytes, ULONGLONG* wct, ULONGLONG* rct);
	//获取指定进程的IO计数
	int		GetIOBytes(DWORD lProcessID, ULONGLONG* read_bytes, ULONGLONG* write_bytes, ULONGLONG* wct, ULONGLONG* rct);

	//获取当前进程的内存
	BOOL	GetMemoryUsed(DWORD& dwPeakWorkingSetSize, DWORD& dwWorkingSetSize);
	//获取指定进程的内存
	BOOL	GetMemoryUsed(DWORD lProcessID, DWORD& dwPeakWorkingSetSize, DWORD& dwWorkingSetSize);

	//获取句柄数
	BOOL	GetHandleCount(DWORD& dwHandles);
	BOOL	GetHandleCount(DWORD lProcessID, DWORD& dwHandles);

	//获取线程数
	BOOL	GetThreadCount(DWORD& dwThreads);
	BOOL	GetThreadCount(DWORD lProcessID, DWORD& dwThreads);

protected:
	//获取指定进程的cpu使用率
	BOOL    GetCPUUserRateEx(HANDLE hProccess, double& dCPUUserRate);

	//获取指定进程的IO计数
	int		GetIOBytesEx(HANDLE hProccess, ULONGLONG* read_bytes, ULONGLONG* write_bytes, ULONGLONG* wct, ULONGLONG* rct);

	//获取内存
	//参数：hProccess：进程句柄；dwPeakWorkingSetSize:使用内存高峰;dwWorkingSetSize:当前使用的内存;
	BOOL	GetMemoryUsedEx(HANDLE hProccess, DWORD& dwPeakWorkingSetSize, DWORD& dwWorkingSetSize);

	//获取句柄数
	BOOL	GetHandleCountEx(HANDLE hProccess, DWORD& dwHandles);
};
