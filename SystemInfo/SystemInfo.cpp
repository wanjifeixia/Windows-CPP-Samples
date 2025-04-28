// SystemInfo2.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "CpuUsage.h"
#include <iostream>
using namespace std;

namespace std
{
#ifdef UNICODE
	typedef wstring string_t;
#else
	typedef string string_t;
#endif

	typedef basic_string<unsigned char, char_traits<unsigned char>, allocator<unsigned char>> bytes;
}

string_t strLog;
HANDLE hThread = NULL;
BOOL bStop = FALSE;


#ifdef STD_CLEAR
#undef STD_CLEAR
#endif
#define STD_CLEAR(cnt) (cnt).clear()

bool UnicodeToGBK(CONST wstring& unicode, string& gbk)
{
	UINT uCodePage = GetACP();
	int lenA = WideCharToMultiByte(uCodePage, 0, unicode.c_str(), (INT)unicode.length(), NULL, 0, NULL, NULL);
	if (lenA > 0)
	{
		gbk.resize(lenA);
		lenA = WideCharToMultiByte(uCodePage, 0, unicode.c_str(), (INT)unicode.length(), (LPSTR)gbk.data(), lenA, NULL, NULL);
		if (lenA > 0)
		{
			gbk.resize(lenA);
			return true;
		}
	}
	STD_CLEAR(gbk);
	return false;
}

string UnicodeToGBK(CONST wstring& unicode)
{
	std::string gbk;
	UnicodeToGBK(unicode, gbk);
	return std::move(gbk);
}


DWORD FileWrite(CONST std::string_t& pathFile, CONST LPVOID lpBuffer, DWORD dwSize)
{
	if (pathFile.empty()) return 0;
	if (lpBuffer == NULL) return 0;
	if ((int)dwSize <= 0) return 0;

	HANDLE hFile = CreateFile(pathFile.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,	//FILE_FLAG_WRITE_THROUGH 操作系统不得推迟对文件的写操作
		NULL);
	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) return 0;

	DWORD dwWritten = 0;
	WriteFile(hFile, lpBuffer, dwSize, &dwWritten, NULL);
	CloseHandle(hFile);
	return dwWritten;
}

//写入文件
DWORD FileWrite(CONST std::string_t& pathFile, CONST std::bytes& strIN)
{
	return FileWrite(pathFile, (CONST LPVOID)strIN.data(), strIN.size());
}

UINT MonitorCpu(LPVOID lpVoid)
{
	CCpuUsage CpuUsage;
	CpuUsage.Init();
	CpuUsage.Update();

	SYSTEMTIME st;
	GetLocalTime(&st);
	wchar_t strBeginTime[100] = { 0 };
	swprintf(strBeginTime, sizeof(strBeginTime), L"【开始测试】%4d-%02d-%02d %02d:%02d:%02d\n\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	wprintf(strBeginTime);
	strLog += strBeginTime;

	DWORD dwTimes = 0;
	double dwCPU = 0;
	double dwMemory = 0;
	DWORD dwBeginTime = GetTickCount();

	while (true)
	{
		if (bStop) break;

		Sleep(1000);
		CpuUsage.Update();
		wstring log = CpuUsage.ToLongString();
		SYSTEMTIME st;
		GetLocalTime(&st);
		wchar_t strTime[100] = { 0 };
		swprintf(strTime, sizeof(strTime), L"%4d-%02d-%02d %02d:%02d:%02d\t", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		wprintf(strTime);
		wprintf(log.c_str());
		wprintf(L"\n");
		strLog += strTime;
		strLog += L"\n";
		strLog += log;
		strLog += L"\n";

		dwTimes++;
		dwCPU += CpuUsage.GetValue();
		dwMemory += CpuUsage.m_dwCurMemoryUsage;
	}

	DWORD dwEndTime = GetTickCount();
	GetLocalTime(&st);
	wchar_t strEndTime[100] = { 0 };
	swprintf(strEndTime, sizeof(strEndTime), L"【结束测试】\n%4d-%02d-%02d %02d:%02d:%02d\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	wprintf(strEndTime);
	strLog += strEndTime;
	swprintf(strEndTime, sizeof(strEndTime), L"【测试报告】总用时【%.2f】秒,平均CPU使用率【%.2f％】,平均内存使用率【%.2f％】\n", (double)(dwEndTime - dwBeginTime) / 1000, dwCPU / dwTimes, dwMemory / dwTimes);
	wprintf(strEndTime);
	strLog += strEndTime;
	return 0;
}

//开始监控CPU、内存等
void StartMonitorCpu()
{
	bStop = FALSE;
	strLog.clear();
	hThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MonitorCpu, NULL, 0, NULL);
}

//停止监控CPU、内存等
void StopMonitorCpu()
{
	bStop = TRUE;
	WaitForSingleObject(hThread, 5000);
	CloseHandle(hThread);
	FileWrite(L"MonitorCpu.log", (std::bytes&)UnicodeToGBK(strLog));
}

BOOL EnableDebugPrivilege() {
	HANDLE hToken;
	BOOL fOk = FALSE;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);

		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);

		fOk = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}
	return fOk;
}

int main()
{
	EnableDebugPrivilege();

	setlocale(LC_ALL, "chs");
	StartMonitorCpu();

	char a;
	cin >> a;
	StopMonitorCpu();
	getchar();
}