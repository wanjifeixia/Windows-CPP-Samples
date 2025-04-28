#pragma once
#include<Windows.h>
#include<winternl.h>
#include<map>
#include<string>
#include<tchar.h>

class IMonitor
{
public:
	virtual ~IMonitor() {}
	virtual const std::basic_string<TCHAR> ToString()const = 0;
	virtual const std::wstring ToLongString()const = 0;
	virtual const double GetValue()const = 0;
	virtual bool Init() = 0;
	virtual void Update() = 0;
	virtual void Reset() = 0;
};

class CCpuUsage :
	public IMonitor
{
	typedef struct tagPROCESS_INFO
	{
		TCHAR name[MAX_PATH];
		unsigned long long last_time, last_system_time;
		double cpu_usage;
		size_t WorkingSetSize;
	}PROCESS_INFO;

public:
	CCpuUsage( );
	~CCpuUsage( );
	
	const std::basic_string<TCHAR> ToString( )const;
	const std::wstring ToLongString( )const;
	const double GetValue()const;
	bool Init( ){ return true; }
	void Update();
	void Reset( );
private:
	unsigned long long __FileTime2Utc(const FILETIME &);
	double __GetCpuUsage(unsigned long long &, unsigned long long &);
	double __GetCpuUsage(HANDLE,unsigned long long &, unsigned long long &);
	void __LoopForProcesses();
	const DWORD __GetCpuConut( )const;
	const std::wstring __Bytes2String(unsigned long long)const;
public:
	double m_dMax, m_dMin, m_dCur;
	DWORD m_dwCurMemoryUsage;
	const DWORD dwNUMBER_OF_PROCESSORS;
	unsigned long long m_ullLastTime, m_ullLastIdleTime;
	std::map<DWORD, PROCESS_INFO> m_mapProcessMap;
	std::pair<DWORD, PROCESS_INFO> m_pairMaxProcesses[3];
};

