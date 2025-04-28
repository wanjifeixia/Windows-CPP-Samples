#include "CpuUsage.h"
#include<TlHelp32.h>
#include<sstream>
#include<iomanip>
#include<Psapi.h>

CCpuUsage::CCpuUsage( )
:m_dMax(-1)
, m_dMin(101)
, m_dCur(0)
, dwNUMBER_OF_PROCESSORS(__GetCpuConut( ))
, m_ullLastTime(0)
, m_ullLastIdleTime(0)
{
}

CCpuUsage::~CCpuUsage( )
{
}

const std::basic_string<TCHAR> CCpuUsage::ToString( )const
{
	std::basic_ostringstream<TCHAR> oss;
	oss << _T("C P U 使用率:") << std::setw(6) << std::setprecision(2)
		<< std::fixed << m_dCur << _T("％");
	return oss.str( );
}

const std::wstring CCpuUsage::ToLongString( )const
{
	std::wostringstream ret;
	ret << L"CPU使用率:" << std::setw(6) << std::setprecision(2) << std::fixed << m_dCur << _T("％")
		<< L"\t内存使用率:" << std::setw(6) << std::setprecision(2) << std::fixed << m_dwCurMemoryUsage << _T("％")
		<< std::setw(5) << L"\n进程ID\t"
		<< std::setw(8) << L"CPU使用率\t工作集使用量"
		<< L"\t进程名" << std::endl;
	for (auto & proc : m_mapProcessMap)
		ret << std::setw(6) << proc.first
		<< std::setw(9) << proc.second.cpu_usage << L"%\t\t" << __Bytes2String(proc.second.WorkingSetSize) << std::setw(5) <<L"\t"
		<< proc.second.name << std::endl;
	return ret.str( );
}

const double CCpuUsage::GetValue( )const
{
	return m_dCur;
}

void CCpuUsage::Update( )
{
	double usage = __GetCpuUsage(m_ullLastTime, m_ullLastIdleTime);
	if (usage >= 0)
	{
		m_dCur = usage;
		if (m_dCur > m_dMax)m_dMax = m_dCur;
		if (m_dCur < m_dMin)m_dMin = m_dCur;
	}

	MEMORYSTATUSEX msex = { sizeof(MEMORYSTATUSEX) };
	if (GlobalMemoryStatusEx(&msex))
	{
		m_dwCurMemoryUsage = msex.dwMemoryLoad;
	}

	__LoopForProcesses( );

}

unsigned long long CCpuUsage::__FileTime2Utc(const FILETIME &ft)
{
	LARGE_INTEGER li;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	return li.QuadPart;
}

double CCpuUsage::__GetCpuUsage(unsigned long long &ullLastTime, unsigned long long &ullLastIdleTime)
{
	FILETIME idleTime, kernelTime, userTime;
	if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
	{
		return -1;
	}
	unsigned long long _time = __FileTime2Utc(kernelTime) + __FileTime2Utc(userTime),
		_idle_time = __FileTime2Utc(idleTime);
	if (!ullLastTime || !ullLastIdleTime)
	{
		ullLastIdleTime = _idle_time;
		ullLastTime = _time;
		return -1;
	}
	unsigned long long idle = _idle_time - ullLastIdleTime,
		usage = _time - ullLastTime;
	ullLastIdleTime = _idle_time;
	ullLastTime = _time;
	if (!usage)
		return -1;
	return ( usage - idle )*100.0 / usage;
}

double CCpuUsage::__GetCpuUsage(HANDLE hProcess, unsigned long long&last_time, unsigned long long&last_system_time)
{
	FILETIME now, creation_time, exit_time, kernel_time, user_time;
	unsigned long long system_time, time, system_time_delta, time_delta;

	GetSystemTimeAsFileTime(&now);

	if (!GetProcessTimes(hProcess, &creation_time, &exit_time, &kernel_time, &user_time))
	{
		return -1;
	}
	system_time = __FileTime2Utc(kernel_time) + __FileTime2Utc(user_time);
	time = __FileTime2Utc(now);
	if (!last_system_time || !last_time)
	{
		last_system_time = system_time;
		last_time = time;
		return -1;
	}
	system_time_delta = system_time - last_system_time;
	time_delta = time - last_time;
	if (!time_delta)
		return -1;
	last_system_time = system_time;
	last_time = time;
	return system_time_delta*100.0 / time_delta / dwNUMBER_OF_PROCESSORS;
}



void CCpuUsage::__LoopForProcesses( )
{
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hProcessSnap)
	{
		return;
	}
	BOOL b;
	PROCESSENTRY32 pe32 = { sizeof( PROCESSENTRY32 ) };
	std::map<DWORD, PROCESS_INFO> oldPi;
	for (auto &x : m_pairMaxProcesses)
		x.second.cpu_usage = 0.0;
	m_mapProcessMap.swap(oldPi);
	for (b = Process32First(hProcessSnap, &pe32); b; b = Process32Next(hProcessSnap, &pe32))
	{
		if (!pe32.th32ProcessID)continue;

		if (!lstrcmpi(pe32.szExeFile, L"dwm.exe")
			|| !lstrcmpi(pe32.szExeFile, L"System")
			|| !lstrcmpi(pe32.szExeFile, L"smss.exe")
			|| !lstrcmpi(pe32.szExeFile, L"services.exe")
			|| !lstrcmpi(pe32.szExeFile, L"vmware-vmx.exe"))
		{
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
			if (!hProcess)
			{
				if (ERROR_ACCESS_DENIED != GetLastError())
				{
					return;
				}
			}
			else
			{
				PROCESS_INFO tp = { 0 };
				auto it = oldPi.find(pe32.th32ProcessID);
				if (it != oldPi.end())
					tp = it->second;
				else
					_tcscpy_s(tp.name, pe32.szExeFile);
				double usage = __GetCpuUsage(hProcess, tp.last_time, tp.last_system_time);
				if (usage >= 0)
					tp.cpu_usage = usage;

				PROCESS_MEMORY_COUNTERS pmc = { sizeof(PROCESS_MEMORY_COUNTERS) };
				if (GetProcessMemoryInfo(hProcess, &pmc, sizeof pmc))
				{
					tp.WorkingSetSize = pmc.WorkingSetSize;
				}
				CloseHandle(hProcess);

				m_mapProcessMap.insert(std::make_pair(pe32.th32ProcessID, tp));
			}
		}
	}
	CloseHandle(hProcessSnap);
}

void CCpuUsage::Reset( )
{
	m_dMax = -1;
	m_dMin = 101;
	m_dCur = 0;
	m_ullLastTime = 0;
	m_ullLastIdleTime = 0;
}

const DWORD CCpuUsage::__GetCpuConut( )const
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

const std::wstring CCpuUsage::__Bytes2String(unsigned long long sz)const
{
	std::wostringstream ret;
	if (sz < 1024) ret << sz << L" B";
	else if (sz < 1024 * 1024) ret << std::setprecision(2) << std::fixed << sz / 1024.0 << L" KB";
	else if (sz < 1024 * 1024 * 1024)ret << std::setprecision(2) << std::fixed << sz / 1024.0 / 1024.0 << L" MB";
	else if (sz < 1024ULL * 1024 * 1024 * 1024)ret << std::setprecision(2) << std::fixed << sz / 1024.0 / 1024.0 / 1024.0 << L" GB";
	else ret << std::setprecision(2) << std::fixed << sz / 1024.0 / 1024.0 / 1024.0 / 1024.0 << L" TB";
	return ret.str();
}