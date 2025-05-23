// CSystemStatus.h: interface for the CSystemStatus class.---chenxiang---2019-08-08----
#ifndef __CSystemStatus_h__
#define __CSystemStatus_h__
#include <vector>
#include <string>

typedef struct tagEACHDISKSTATUS
{
	std::string _strdir;			//磁盘名称
	float	_Total;					//磁盘总容量MB
	float	_OfFree;				//磁盘剩余容量MB
}EACHDISKSTATUS, * LPEACHDISKSTATUS;

 
#include "pdh.h"
 
class CSystemStatus
{
public:
	CSystemStatus();
	~CSystemStatus();
public:
	void		SystemInit(DWORD object = SYSSTATE_CPU_USAGE);							//系统初始化(初始化多个项目时使用或运算连接)
	void		SystemUnInit();															//释放资源
	double		GetSystemNetDownloadRate();												//获取网络下载速度
	double		GetSystemNetUploadRate();												//获取网络上传速度
	double		GetSystemDiskReadRate();												//获取当前磁盘读速率
	double		GetSystemDiskWriteRate();												//获取当前磁盘写速率
	double		GetSystemCpuCurrentUsage();												//系统CPU使用率

	void		GetSystemDiskStatus(std::vector<EACHDISKSTATUS> &vectorDisk);           //获取各个磁盘使用状态
	void		GetSystemDiskStatus(ULONGLONG& AllDiskTotal, ULONGLONG& AllDiskFree);	//获取系统总得磁盘使用状态
	void		GetSystemCurrentDiskStatus(ULONGLONG& TatolMB, ULONGLONG& FreeCaller);	//获取当前磁盘使用状态
	double		GetSystemCurrentDiskUsage();											//获取当前磁盘使用率

	BOOL		GetPhysicalMemoryState(ULONGLONG& totalPhysMem, ULONGLONG& physMemUsed);//获取物理内存状态
	double		GetTotalPhysicalMemory();												//获取可用内存大小
	double		GetTotalPhysicalMemoryFree();											//获取空闲内存
	double		GetTotalPhysicalMemoryUsed();											//获取已使用内存大小
	double		GetPhysicalMemoryUsage();												//获取内存使用率

	void		GetNetCardInfo(std::vector<NETCARDINFO> &vectorNetCard);				//获取网卡信息
	void		GetOsInfo(std::string &osinfo);                                         //获取操作系统信息 
	void		GetCpuInfo(std::string &CPUinfo);										//获取CPU硬件信息 	
	void		GetCPUid(std::string &CPUid);											//获取CPUid

	BOOL		GetHDSerial(std::string &HDSerial);										//获取硬盘物理序列号（需要管理员权限）
private:
	PDH_HQUERY		m_Query; 
	PDH_HCOUNTER	m_CpuTotal,m_DiskRead,m_DiskWrite,m_NetDownload,m_NetUpload;

public:
	/*
	参数:const char *cmd
	systeminfo:查看详细的系统信息
	wmic logicaldisk:查看盘符
	fsutil volume diskfree + 盘符名称:查看某个盘符的容量大小。
	wmic path win32_physicalmedia get SerialNumber;查看硬盘系列号
	wmic diskdrive get serialnumber;查看硬盘系列号(和上面效果一样)
	wmic cpu:查看CPU运行信息
	wmic cpu list brief:查看CPU硬件信息
	wmic memorychip;查看系统内存信息
	wmic bios:查看系统的bios信息
	wmic memorychip list brief:查看内存条数
	wmic memcache list brief:查看缓存内存
	wmic diskdrive:查看磁盘详细信息
	wmic diskdrive get Name, Model:查看硬盘名称，型号（使用get）
	...
	*/
	std::string execCmd(const char *cmd)
	{
		char buffer[128] = { 0 };
		std::string result;
		FILE *pipe = _popen(cmd, "r");
		if (!pipe) throw std::runtime_error("_popen() failed!");
		while (!feof(pipe))
		{
			if (fgets(buffer, 128, pipe) != NULL)
				result += buffer;
		}
		_pclose(pipe);
		return result;
	}

	_inline void ChangeByteOrder(PCHAR szString, USHORT uscStrSize) 
	{ 
		USHORT  i	= 0; 
		CHAR	temp= '\0'; 

		for (i = 0; i < uscStrSize; i+=2) 
		{ 
			temp			= szString[i]; 
			szString[i]		= szString[i+1]; 
			szString[i+1]	= temp; 
		} 
	}
};
#endif