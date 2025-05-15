// main.cpp
#include "../MyDll/MyDll.h"
#include <windows.h>
#include <iostream>
#include <winusb.h>
#include <ShlObj_core.h>

#pragma comment(lib, "../x64/Release/MyDll.lib")

//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) 

typedef BOOL(WINAPI* WinUsbInitializeFunc)(HANDLE, PWINUSB_INTERFACE_HANDLE);

int main()
{
	std::cout << "[TestFirstLoadDll.exe] Enter main, Begin Call MyCreateEvent\n" << std::endl;

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

		std::wcout << L"输入1 = 调用MyDll.dll导出函数MyCreateEvent" << std::endl;
		std::wcout << L"输入2 = 开始动态加载 [winusb.dll]" << std::endl;
		std::wcout << L"输入3 = GetCurrentDirectory获取当前目录并加载当前目录下的Dll" << std::endl;
		std::wcout << L"输入4 = SHGetFolderPath获取公共桌面" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			int ret = MyCreateEvent();
		}
		else if (iInput == 2)
		{
			std::cout << "[TestFirstLoadDll.exe] Begin LoadLibrary winusb.dll" << std::endl;

			// 加载 winusb.dll 库
			HMODULE hModule = LoadLibrary(L"winusb.dll");
			if (hModule == NULL) {
				std::cerr << "Failed to load winusb.dll" << std::endl;
				continue;
			}

			// 获取 WinUsb_Initialize 函数的地址
			WinUsbInitializeFunc WinUsb_Initialize = (WinUsbInitializeFunc)GetProcAddress(hModule, "WinUsb_Initialize");
			if (WinUsb_Initialize == NULL) {
				std::cerr << "Failed to get address of WinUsb_Initialize" << std::endl;
				FreeLibrary(hModule);
				continue;
			}

			// 假设我们有一个已打开的 USB 设备句柄
			HANDLE hDevice = INVALID_HANDLE_VALUE;  // 设备句柄应由你自己获取
			WINUSB_INTERFACE_HANDLE hWinUsb;

			// 调用 WinUsb_Initialize
			std::cout << "[TestFirstLoadDll.exe] Begin Call WinUsb_Initialize" << std::endl;
			BOOL result = WinUsb_Initialize(hDevice, &hWinUsb);
			if (result) {
				std::cout << "WinUsb_Initialize succeeded." << std::endl;
			}

			FreeLibrary(hModule);
		}
		else if (iInput == 3)
		{
			// 获取当前工作目录
			WCHAR currentDirectory[MAX_PATH];
			DWORD length = GetCurrentDirectory(MAX_PATH, currentDirectory);

			if (length == 0) {
				std::cerr << "无法获取当前目录，错误代码：" << GetLastError() << std::endl;
				continue;
			}

			std::wcout << "currentDirectory：" << currentDirectory << std::endl;

			// 构造DLL路径
			std::wstring dllPath = std::wstring(currentDirectory) + L"\\gdi111.dll";

			// 加载DLL
			HMODULE hModule = LoadLibraryW(dllPath.c_str());
			if (hModule == NULL) {
				std::wcout << L"Failed DLL：" << dllPath << std::endl;
			}
			else
			{
				std::wcout << L"Success DLL：" << dllPath << std::endl;
			}
		}
		else if (iInput == 4)
		{
			TCHAR path[MAX_PATH];
			HRESULT result = SHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, path);

			if (SUCCEEDED(result)) {
				std::wcout << L"Public Desktop Path: " << path << std::endl;
			}
			else {
				std::cerr << "Failed to get the public desktop path." << std::endl;
			}
		}
	}
}