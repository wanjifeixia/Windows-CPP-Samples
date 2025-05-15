#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <iphlpapi.h>
#include <Psapi.h>
#include <lm.h>
#include <vector>

#pragma comment(lib, "mpr.lib")
#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib") 

//获取使用SMB协议的进程
void GetSMBExplorerProcesses()
{
	// Step 1: Enumerate TCP connections and find SMB connections
	MIB_TCPTABLE_OWNER_PID tcpTable;
	DWORD dwSize = sizeof(tcpTable);

	if (GetExtendedTcpTable((PVOID)&tcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == ERROR_INSUFFICIENT_BUFFER)
	{
		// Allocate memory for the TCP table
		PMIB_TCPTABLE_OWNER_PID pTcpTable = (PMIB_TCPTABLE_OWNER_PID)malloc(dwSize);

		if (pTcpTable != NULL)
		{
			if (GetExtendedTcpTable((PVOID)pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR)
			{
				// Step 2: Find the SMB connections and get their associated PID (Process ID)
				for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++)
				{
					if (pTcpTable->table[i].dwRemotePort == htons(445))
					{
						DWORD dwPID = pTcpTable->table[i].dwOwningPid;
						std::wcout << "使用SMB协议的进程 PID: " << dwPID << std::endl;
					}
				}
			}
			free(pTcpTable);
		}
	}
}

// 结构体用于存储NAS连接信息
struct NASConnection {
	std::wstring localName;   // 例如 "Z:"
	std::wstring remoteName;   // 例如 "\\\\192.168.1.100\\share"
	std::wstring comment;
};

std::vector<NASConnection> EnumerateNASConnections() {
	std::vector<NASConnection> connections;

	DWORD result;
	HANDLE hEnum;
	DWORD cbBuffer = 16384; // 建议的缓冲区大小
	LPNETRESOURCEW lpnr = NULL;

	// 开始枚举网络资源
	result = WNetOpenEnumW(
		RESOURCE_CONNECTED,    // 枚举已连接的资源
		RESOURCETYPE_ANY,      // 仅枚举磁盘类型
		0,                     // 所有资源
		NULL,                  // 首次枚举不需要资源句柄
		&hEnum
	);

	if (result != NO_ERROR) {
		std::wcerr << L"WNetOpenEnum 失败。错误代码: " << result << std::endl;
		return connections;
	}

	// 分配缓冲区
	lpnr = (LPNETRESOURCEW)GlobalAlloc(GPTR, cbBuffer);

	if (lpnr == NULL) {
		WNetCloseEnum(hEnum);
		return connections;
	}

	DWORD cEntries = 0xFFFFFFFF; // 尽可能多地获取条目
	DWORD retCode;

	do {
		// 枚举资源
		retCode = WNetEnumResourceW(hEnum, &cEntries, lpnr, &cbBuffer);

		if (retCode == NO_ERROR) {
			for (DWORD i = 0; i < cEntries; i++) {
				NASConnection conn;
				if (lpnr[i].lpLocalName) conn.localName = lpnr[i].lpLocalName;
				if (lpnr[i].lpRemoteName) conn.remoteName = lpnr[i].lpRemoteName;
				if (lpnr[i].lpComment) conn.comment = lpnr[i].lpComment;

				connections.push_back(conn);
			}
		}
		else if (retCode != ERROR_NO_MORE_ITEMS) {
			std::wcerr << L"WNetEnumResource 失败。错误代码: " << retCode << std::endl;
			break;
		}
	} while (retCode != ERROR_NO_MORE_ITEMS);

	// 清理资源
	GlobalFree(lpnr);
	WNetCloseEnum(hEnum);

	return connections;
}

// 映射网络驱动器
bool MapNetworkDrive(const wchar_t* remotePath,
	const wchar_t* localDrive,
	const wchar_t* username = nullptr,
	const wchar_t* password = nullptr)
{
	NETRESOURCEW netResource = { 0 };
	netResource.dwType = RESOURCETYPE_DISK;
	netResource.lpLocalName = (LPWSTR)localDrive;
	netResource.lpRemoteName = const_cast<wchar_t*>(remotePath);
	netResource.lpProvider = nullptr;

	DWORD result = WNetAddConnection2W(
		&netResource,
		password,
		username,
		CONNECT_UPDATE_PROFILE
		// CONNECT_TEMPORARY临时连接（重启后失效）
		// 使用 CONNECT_UPDATE_PROFILE 可使映射持久化
	);

	if (result == NO_ERROR) {
		std::wcout << L"成功映射 " << remotePath
			<< L" 到 " << localDrive << std::endl;
		return true;
	}
	else {
		std::wcerr << L"映射失败 (错误码: 0x" << std::hex << result << L") ";
		switch (result) {
		case ERROR_ACCESS_DENIED:
			std::wcerr << L"权限不足"; break;
		case ERROR_ALREADY_ASSIGNED:
			std::wcerr << L"驱动器已分配"; break;
		case ERROR_BAD_NETPATH:
			std::wcerr << L"网络路径无效"; break;
		case ERROR_BAD_NET_NAME:
			std::wcerr << L"网络名称格式错误"; break;
		case ERROR_INVALID_PASSWORD:
			std::wcerr << L"密码错误"; break;
		default:
			std::wcerr << L"未知错误";
		}
		std::wcerr << std::endl;
		return false;
	}
}

// 卸载网络驱动器
bool UnmapNetworkDrive(const wchar_t* localDrive) {
	DWORD result = WNetCancelConnection2W(
		localDrive,
		CONNECT_UPDATE_PROFILE,  // 更新用户配置
		TRUE                     // 强制卸载
	);

	if (result == NO_ERROR) {
		std::wcout << localDrive << L" 卸载成功" << std::endl;
		return true;
	}
	else {
		std::wcerr << L"卸载失败 (错误码: 0x" << std::hex << result << L")" << std::endl;
		return false;
	}
}

int main()
{
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));
	std::wcout.imbue(std::locale(""));

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = CreateFile打开Nas中的文件: \\\\192.168.207.130\\NasFile\\tools\\111.txt" << std::endl;
		std::wcout << L"输入2 = NetUseEnum枚举当前连接" << std::endl;
		std::wcout << L"输入3 = WNetEnumResourceW枚举当前连接" << std::endl;
		std::wcout << L"输入4 = WNetAddConnection2W连接192.168.207.130并枚举文件" << std::endl;
		std::wcout << L"输入5 = WNetCancelConnection2W断开连接192.168.207.130" << std::endl;
		std::wcout << L"输入6 = 获取使用SMB协议的进程" << std::endl;
		std::wcout << L"输入7 = 映射网络驱动器\\\\192.168.207.130\\NasFile到[W盘]" << std::endl;
		std::wcout << L"输入8 = 卸载网络驱动器[W盘]" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			const char* networkPath = R"(\\192.168.207.130\NasFile\tools\111.txt)";

			// 打开文件
			HANDLE hFile = CreateFileA(
				networkPath,
				GENERIC_READ,             // 只读
				FILE_SHARE_READ,          // 允许其他进程读取
				nullptr,                  // 默认安全属性
				OPEN_EXISTING,            // 打开已有文件
				FILE_ATTRIBUTE_NORMAL,    // 常规文件属性
				nullptr                   // 无模板文件
			);

			if (hFile == INVALID_HANDLE_VALUE) {
				std::cerr << "Failed to open file. Error: " << GetLastError() << std::endl;
				continue;
			}

			std::cout << "File opened successfully!" << std::endl;

			// 关闭文件句柄
			CloseHandle(hFile);
		}
		else if (iInput == 2)
		{
			USE_INFO_2* useInfo;
			DWORD entriesRead, totalEntries;
			NET_API_STATUS status = NetUseEnum(nullptr, 2, (LPBYTE*)&useInfo, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr);
			if (status == NERR_Success)
			{
				for (DWORD i = 0; i < entriesRead; i++)
				{
					std::wcout << "Local Name: " << (LPCWSTR)useInfo[i].ui2_local << std::endl;
					std::wcout << "Remote Name: " << (LPCWSTR)useInfo[i].ui2_remote << std::endl;
					std::wcout << "Username: " << (LPCWSTR)useInfo[i].ui2_username << std::endl << std::endl;
				}
				NetApiBufferFree(useInfo);
			}
		}
		else if (iInput == 3)
		{
			std::vector<NASConnection> nasList = EnumerateNASConnections();
			for (const auto& conn : nasList) {
				std::wcout << L"本地名称: " << conn.localName << std::endl;
				std::wcout << L"远程路径: " << conn.remoteName << std::endl;
				std::wcout << L"描述信息: " << conn.comment << std::endl;
				std::wcout << L"------------------------" << std::endl;
			}
		}
		else if (iInput == 4)
		{
			// 连接到共享文件夹
			const wchar_t* sharedFolderPath = L"\\\\192.168.207.130";
			NETRESOURCEW nr;
			nr.dwType = RESOURCETYPE_DISK;
			nr.lpLocalName = nullptr;
			nr.lpRemoteName = const_cast<wchar_t*>(sharedFolderPath);
			nr.lpProvider = nullptr;

			DWORD result = WNetAddConnection2W(&nr, L"1qaz", L"46043", CONNECT_UPDATE_PROFILE);
			if (result != NO_ERROR)
			{
				std::cout << "连接失败，错误代码: " << result << std::endl;
				continue;
			}

			// 列出共享文件夹中的文件
			std::cout << "连接成功" << std::endl;
			WIN32_FIND_DATAW findData;
			HANDLE hFind = FindFirstFileW((sharedFolderPath + std::wstring(L"\\NasFile\\*")).c_str(), &findData);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do {
					if (wcscmp(findData.cFileName, L".") && wcscmp(findData.cFileName, L".."))
						std::wcout << findData.cFileName << std::endl;
				} while (FindNextFileW(hFind, &findData) != 0);

				FindClose(hFind);
			}
		}
		else if (iInput == 5)
		{
			std::vector<NASConnection> nasList = EnumerateNASConnections();
			for (const auto& conn : nasList) {
				// 断开与共享文件夹的连接
				DWORD dwResult = WNetCancelConnection2W(conn.remoteName.c_str(), CONNECT_UPDATE_PROFILE, TRUE);
				if (dwResult)
					std::wcout << L"WNetCancelConnection2W failed：" << conn.remoteName.c_str() << L" 错误码:" << dwResult << std::endl;
				else
					std::wcout << L"WNetCancelConnection2W success：" << conn.remoteName.c_str() << std::endl;
			}
			
		}
		else if (iInput == 6)
		{
			//获取使用SMB协议的进程
			GetSMBExplorerProcesses();
		}
		else if (iInput == 7)
		{
			// 配置参数
			const wchar_t* nasPath = L"\\\\192.168.207.130\\NasFile";
			const wchar_t* driveLetter = L"W:";
			const wchar_t* username = L"46043"; // 如果需要认证
			const wchar_t* password = L"1qaz";

			//映载网络驱动器
			MapNetworkDrive(nasPath, driveLetter, username, password);
		}
		else if (iInput == 8)
		{
			//卸载网络驱动器
			const wchar_t* driveLetter = L"W:";
			UnmapNetworkDrive(driveLetter);
		}
	}
	return 0;
}