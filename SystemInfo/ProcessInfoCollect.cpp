#include "ProcessInfoCollect.h"
#include <stdio.h>
#include <tchar.h>
#include <TLHELP32.H>
#include <winsvc.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")

#ifdef StartProcessBySysServiceEx_Flag
#include <Userenv.h>
#include <WtsApi32.h>
#include <atlbase.h>
#pragma comment(lib, "WtsApi32.lib")
#pragma comment(lib,"Userenv.lib")
#endif


#define			MAX_ID							4096			//��������
#define			MAX_CHILD_PROCESS_COUNT			256				//�ӽ�����

//�õ��ļ���(������չ��)  
const char* GetFileName(const char* pFile)
{
	if (NULL == pFile || 0 == strlen(pFile))
	{
		return "";
	}

	const char* pPos = strrchr(pFile, '\\');
	if (NULL == pPos)
	{
		pPos = strrchr(pFile, '/');

		if (NULL == pPos)
		{
			return "";
		}
	}

	return pPos + 1;
}

#ifdef StartProcessBySysServiceEx_Flag
DWORD GetActiveSessionID()
{
	DWORD dwSessionId = 0;
	PWTS_SESSION_INFO pSessionInfo = NULL;
	DWORD dwCount = 0;
	WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwCount);
	for (DWORD i = 0; i < dwCount; i++)
	{
		WTS_SESSION_INFO si = pSessionInfo[i];
		if (WTSActive == si.State)
		{
			dwSessionId = si.SessionId;
			break;
		}
	}
	WTSFreeMemory(pSessionInfo);
	return dwSessionId;
}
#endif

ProcessInfoCollect::ProcessInfoCollect()
{
}

ProcessInfoCollect::~ProcessInfoCollect()
{
}

//��ȡcpuʹ����
BOOL  ProcessInfoCollect::GetCPUUserRate(double& dCPUUserRate)
{
	HANDLE hProcess = ::GetCurrentProcess();
	return GetCPUUserRateEx(hProcess, dCPUUserRate);
}

//��ȡָ�����̵�cpuʹ����
BOOL    ProcessInfoCollect::GetCPUUserRate(DWORD lProcessID, double& dCPUUserRate)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lProcessID);
	if (hProcess == NULL)
		return FALSE;

	BOOL bSuccess = GetCPUUserRateEx(hProcess, dCPUUserRate);
	CloseHandle(hProcess);
	return bSuccess;
}

int ProcessInfoCollect::GetIOBytes(ULONGLONG* read_bytes, ULONGLONG* write_bytes, ULONGLONG* wct, ULONGLONG* rct)
{
	HANDLE hProcess = GetCurrentProcess();//��ȡ��ǰ���̾��
	int nRet = GetIOBytesEx(hProcess, read_bytes, write_bytes, wct, rct);
	return nRet;
}
//��ȡָ�����̵�IO����
int	ProcessInfoCollect::GetIOBytes(DWORD lProcessID, ULONGLONG* read_bytes, ULONGLONG* write_bytes, ULONGLONG* wct, ULONGLONG* rct)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lProcessID);
	if (hProcess == NULL)
		return -1;

	int nRet = GetIOBytesEx(hProcess, read_bytes, write_bytes, wct, rct);
	CloseHandle(hProcess);
	return nRet;
}

//��ȡ�����
BOOL	ProcessInfoCollect::GetHandleCount(DWORD& dwHandles)
{
	return GetHandleCountEx(GetCurrentProcess(), dwHandles);
}
BOOL	ProcessInfoCollect::GetHandleCount(DWORD lProcessID, DWORD& dwHandles)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lProcessID);
	if (hProcess == NULL)
		return FALSE;

	BOOL bSuccess = GetHandleCountEx(hProcess, dwHandles);
	CloseHandle(hProcess);
	return bSuccess;
}

//��ȡ��ǰ���̵��ڴ�
BOOL	ProcessInfoCollect::GetMemoryUsed(DWORD& dwPeakWorkingSetSize, DWORD& dwWorkingSetSize)
{
	HANDLE hProcess = GetCurrentProcess();//��ȡ��ǰ���̾��
	return GetMemoryUsedEx(hProcess, dwPeakWorkingSetSize, dwWorkingSetSize);
}
//��ȡָ�����̵��ڴ�
BOOL	ProcessInfoCollect::GetMemoryUsed(DWORD lProcessID, DWORD& dwPeakWorkingSetSize, DWORD& dwWorkingSetSize)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, lProcessID);
	if (hProcess == NULL)
		return FALSE;

	BOOL bSuccess = GetMemoryUsedEx(hProcess, dwPeakWorkingSetSize, dwWorkingSetSize);
	CloseHandle(hProcess);
	return bSuccess;
}

//��ȡָ�����̵�cpuʹ����
BOOL    ProcessInfoCollect::GetCPUUserRateEx(HANDLE hProccess, double& dCPUUserRate)
{
	static DWORD s_dwTickCountOld = 0;
	static LARGE_INTEGER s_lgProcessTimeOld = { 0 };
	static DWORD s_dwProcessorCoreNum = 0;
	if (!s_dwProcessorCoreNum)
	{
		SYSTEM_INFO sysInfo = { 0 };
		GetSystemInfo(&sysInfo);
		s_dwProcessorCoreNum = sysInfo.dwNumberOfProcessors;
	}
	double dbProcCpuPercent = 0;
	BOOL bRetCode = FALSE;
	FILETIME CreateTime, ExitTime, KernelTime, UserTime;
	LARGE_INTEGER lgKernelTime;
	LARGE_INTEGER lgUserTime;
	LARGE_INTEGER lgCurTime;
	bRetCode = GetProcessTimes(hProccess, &CreateTime, &ExitTime, &KernelTime, &UserTime);
	if (bRetCode)
	{
		lgKernelTime.HighPart = KernelTime.dwHighDateTime;
		lgKernelTime.LowPart = KernelTime.dwLowDateTime;
		lgUserTime.HighPart = UserTime.dwHighDateTime;
		lgUserTime.LowPart = UserTime.dwLowDateTime;
		lgCurTime.QuadPart = (lgKernelTime.QuadPart + lgUserTime.QuadPart);
		if (s_lgProcessTimeOld.QuadPart)
		{
			DWORD dwElepsedTime = ::GetTickCount() - s_dwTickCountOld;
			dbProcCpuPercent = (double)(((double)((lgCurTime.QuadPart - s_lgProcessTimeOld.QuadPart) * 100)) / dwElepsedTime) / 10000;
			dbProcCpuPercent = dbProcCpuPercent / s_dwProcessorCoreNum;
		}
		s_lgProcessTimeOld = lgCurTime;
		s_dwTickCountOld = ::GetTickCount();
	}
	dCPUUserRate = dbProcCpuPercent;
	return bRetCode;
}
//��ȡָ�����̵�IO����
int		ProcessInfoCollect::GetIOBytesEx(HANDLE hProccess, ULONGLONG* read_bytes, ULONGLONG* write_bytes, ULONGLONG* wct, ULONGLONG* rct)
{
	IO_COUNTERS io_counter;
	if (GetProcessIoCounters(hProccess, &io_counter))
	{
		if (read_bytes) *read_bytes = io_counter.ReadTransferCount;//�ֽ���
		if (write_bytes) *write_bytes = io_counter.WriteTransferCount;
		if (wct) *wct = io_counter.WriteOperationCount;//����
		if (rct) *rct = io_counter.ReadOperationCount;
		return 0;
	}
	return -1;
}
#pragma warning(disable: 4996)
//��ȡ�ڴ�
//������hProccess�����̾����dwPeakWorkingSetSize:ʹ���ڴ�߷�;dwWorkingSetSize:��ǰʹ�õ��ڴ�;
BOOL	ProcessInfoCollect::GetMemoryUsedEx(HANDLE hProccess, DWORD& dwPeakWorkingSetSize, DWORD& dwWorkingSetSize)
{
	//���ݽ���ID�򿪽���
	if (hProccess)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		OSVERSIONINFO osvi;//����OSVERSIONINFO���ݽṹ����
		memset(&osvi, 0, sizeof(OSVERSIONINFO));//���ռ� 
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);//�����С 
		GetVersionEx(&osvi);//��ð汾��Ϣ 
		if (osvi.dwMajorVersion < 6)
		{
			PROCESS_MEMORY_COUNTERS pmc;
			pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
			//��ȡ������̵��ڴ�ʹ�������
			if (::GetProcessMemoryInfo(hProccess, &pmc, sizeof(pmc)))
			{
				dwWorkingSetSize = pmc.PagefileUsage;//pmc.WorkingSetSize;
				dwPeakWorkingSetSize = pmc.PeakWorkingSetSize;
				//ȱҳ�жϴ���:pmc.PageFaultCount
				//ʹ���ڴ�߷�:pmc.PeakWorkingSetSize
				//��ǰʹ�õ��ڴ�: pmc.WorkingSetSize
				//ʹ��ҳ�滺��ظ߷�: pmc.QuotaPeakPagedPoolUsage
				//ʹ��ҳ�滺���: pmc.QuotaPagedPoolUsage
				//ʹ�÷Ƿ�ҳ����ظ߷�: pmc.QuotaPeakNonPagedPoolUsage
				//ʹ�÷Ƿ�ҳ�����: pmc.QuotaNonPagedPoolUsage
				//ʹ�÷�ҳ�ļ�:pmc.PagefileUsage
				//ʹ�÷�ҳ�ļ��ĸ߷�: pmc.PeakPagefileUsage
			}
		}
		else
		{
			DWORD dwMemProcess = 0;
			PSAPI_WORKING_SET_INFORMATION workSet;
			memset(&workSet, 0, sizeof(workSet));
			BOOL bOk = QueryWorkingSet(hProccess, &workSet, sizeof(workSet));
			if (bOk || (!bOk && GetLastError() == ERROR_BAD_LENGTH))
			{
				int nSize = sizeof(workSet.NumberOfEntries) + workSet.NumberOfEntries * sizeof(workSet.WorkingSetInfo);
				char* pBuf = new char[nSize];
				if (pBuf)
				{
					QueryWorkingSet(hProccess, pBuf, nSize);
					PSAPI_WORKING_SET_BLOCK* pFirst = (PSAPI_WORKING_SET_BLOCK*)(pBuf + sizeof(workSet.NumberOfEntries));
					DWORD dwMem = 0;
					for (ULONG_PTR nMemEntryCnt = 0; nMemEntryCnt < workSet.NumberOfEntries; nMemEntryCnt++, pFirst++)
					{
						if (pFirst->Shared == 0) dwMem += si.dwPageSize;
					}
					delete pBuf;
					if (workSet.NumberOfEntries > 0)
					{
						dwMemProcess = dwMem;
						dwWorkingSetSize = dwMemProcess;
						dwPeakWorkingSetSize = dwMemProcess;
					}
				}
			}
			else
			{
				return FALSE;
			}
		}
	}
	else
	{
		int ret = GetLastError();
		return FALSE;
	}
	return TRUE;
}

//��ȡ�����
BOOL ProcessInfoCollect::GetHandleCountEx(HANDLE hProccess, DWORD& dwHandles)
{
	return GetProcessHandleCount(hProccess, &dwHandles);
}

//��ȡ�߳���
BOOL	ProcessInfoCollect::GetThreadCount(DWORD& dwThreads)
{
	return GetThreadCount(GetCurrentProcessId(), dwThreads);
}
BOOL ProcessInfoCollect::GetThreadCount(DWORD lProcessID, DWORD& dwThreads)
{
	//��ȡ������Ϣ
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return FALSE;

	BOOL bFind = FALSE;
	char szFilePath[MAX_PATH] = { 0 };
	PROCESSENTRY32 stProcessEntry32 = { 0 };
	stProcessEntry32.dwSize = sizeof(stProcessEntry32);
	BOOL bSucceed = ::Process32First(hProcessSnap, &stProcessEntry32);;
	for (;;)
	{
		if (!bSucceed)
			break;

		if (stProcessEntry32.th32ProcessID == lProcessID)
		{
			dwThreads = stProcessEntry32.cntThreads;
			bFind = TRUE;
			break;
		}
		bSucceed = ::Process32Next(hProcessSnap, &stProcessEntry32);
	}
	::CloseHandle(hProcessSnap);
	return bFind;
}