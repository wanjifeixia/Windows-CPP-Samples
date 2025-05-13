// TestGetDriveType.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
#include <iostream>
#include <fileapi.h>
#include <winnetwk.h>
#include <thread>
#include <future>
#include <string>

#pragma comment(lib, "mpr.lib")

using namespace std;

#define  DRIVE_LETTER_TO_BIT(letter)  (0x2000000 >> (L'Z' - letter))

//多线程异步获取
UINT GetDriveTypeWithInterrupt(const std::string& drive, unsigned int timeoutMs) {
	std::atomic<bool> isDone{ false };
	UINT driveType = DRIVE_UNKNOWN;

	// 子线程执行 GetDriveTypeA
	std::thread worker([&]() {
		driveType = GetDriveTypeA(drive.c_str());
		isDone = true;
		});

	// 等待指定时间
	auto start = std::chrono::high_resolution_clock::now();
	while (!isDone && std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::high_resolution_clock::now() - start)
		.count() < timeoutMs) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 检查间隔
	}

	if (!isDone) {
		std::cerr << "Timeout while getting drive type for: " << drive << std::endl;
	}

	// 确保线程结束
	if (worker.joinable()) {
		worker.detach(); // 超时后分离线程
	}

	return isDone ? driveType : DRIVE_UNKNOWN; // 超时返回 UNKNOWN
}

std::string GetCurrentTimeString(const char* format = "%Y-%m-%d %H:%M:%S")
{
	time_t now = time(nullptr);
	if (now == -1) {
		return "[Time Error]";
	}

	struct tm localTime;
	if (localtime_s(&localTime, &now) != 0) {
		return "[Time Error]";
	}

	char buffer[100];
	if (strftime(buffer, sizeof(buffer), format, &localTime) == 0) {
		return "[Format Error]";
	}

	return buffer;
}

void PrintDriveType(LPCSTR drive, char disk)
{
	//多线程异步获取
	//UINT driveType = GetDriveTypeWithInterrupt(drive, 200);
	
	UINT driveType = DRIVE_UNKNOWN;
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, (string("Network\\") + disk).c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		driveType = DRIVE_REMOTE;
	}
	else
	{
		driveType = GetDriveTypeA(drive);
	}

	printf("%s\tEnd \t", GetCurrentTimeString().c_str());

	switch (driveType) {
	case DRIVE_UNKNOWN:
		printf("Drive %s: UNKNOWN TYPE\n", drive);
		break;
	case DRIVE_NO_ROOT_DIR:
		printf("Drive %s: INVALID ROOT PATH\n", drive);
		break;
	case DRIVE_REMOVABLE:
		printf("Drive %s: Removable Drive\n", drive);
		break;
	case DRIVE_FIXED:
		printf("Drive %s: Fixed Drive (Hard Disk)\n", drive);
		break;
	case DRIVE_REMOTE:
		printf("Drive %s: Network Drive\n", drive);
		break;
	case DRIVE_CDROM:
		printf("Drive %s: CD-ROM Drive\n", drive);
		break;
	case DRIVE_RAMDISK:
		printf("Drive %s: RAM Disk\n", drive);
		break;
	default:
		printf("Drive %s: Unknown Return Value\n", drive);
		break;
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

		std::wcout << L"输入1 = 通过GetLogicalDriveStringsA获取磁盘和类型" << std::endl;
		std::wcout << L"输入2 = 通过GetLogicalDrives获取磁盘和类型" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			char drives[256];
			DWORD driveMask = GetLogicalDriveStringsA(sizeof(drives), drives);

			if (driveMask == 0) {
				printf("Error retrieving drives\n");
				return 1;
			}

			for (char* drive = drives; *drive; drive += strlen(drive) + 1)
			{
				printf("%s\tBegin\tGetDriveType:%s\n", GetCurrentTimeString().c_str(), drive);
				PrintDriveType(drive, drive[0]);
			}
		}
		else if (iInput == 2)
		{
			DWORD Drivers = GetLogicalDrives();
			const char chrs[] = "CDEFGHIJKLMNOPQRSTUVWXYZ";
			for (int i = 0; i < 24; i++)
			{
				string strDisk = chrs[i] + string(":/");
				if (DRIVE_LETTER_TO_BIT(chrs[i]) & Drivers)
				{
					printf("%s\tBegin\tGetDriveType:%s\n", GetCurrentTimeString().c_str(), strDisk.c_str());
					PrintDriveType(strDisk.c_str(), chrs[i]);
				}
			}
		}
	}
	return 0;
}