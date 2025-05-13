// main.cpp
#include "../MyDll/MyDll.h"
#include <windows.h>
#include <iostream>
#include <winusb.h>

#pragma comment(lib, "../x64/Release/MyDll.lib")

//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" ) 

typedef BOOL(WINAPI* WinUsbInitializeFunc)(HANDLE, PWINUSB_INTERFACE_HANDLE);

int main()
{
	std::cout << "[TestFirstLoadDll.exe] Enter main, Begin Call MyCreateEvent" << std::endl;

	int ret = MyCreateEvent();

	std::cout << "输入任何字符开始动态加载 [winusb.dll]" << std::endl;
	getchar();

	std::cout << "[TestFirstLoadDll.exe] Begin LoadLibrary winusb.dll" << std::endl;

	// 加载 winusb.dll 库
	HMODULE hModule = LoadLibrary(L"winusb.dll");
	if (hModule == NULL) {
		std::cerr << "Failed to load winusb.dll" << std::endl;
		return 1;
	}

	// 获取 WinUsb_Initialize 函数的地址
	WinUsbInitializeFunc WinUsb_Initialize = (WinUsbInitializeFunc)GetProcAddress(hModule, "WinUsb_Initialize");
	if (WinUsb_Initialize == NULL) {
		std::cerr << "Failed to get address of WinUsb_Initialize" << std::endl;
		FreeLibrary(hModule);
		return 1;
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

	getchar();
	getchar();

	// 清理
	FreeLibrary(hModule);
	return 0;
}