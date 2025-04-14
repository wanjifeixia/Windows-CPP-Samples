#include <windows.h>
#include <cstdio>
#include <string>
#include <iostream>
using namespace std;

//子窗口回调函数
BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM lParam)
{
	char title[100] = { 0 };
	GetWindowTextA(hwnd, title, 100);
	cout << hex << "子窗口句柄：" << hwnd << "  标题：" << title << endl;
	return true;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	char title[100] = { 0 };
	GetWindowTextA(hwnd, title, 100);
	cout << hex << "主窗口句柄：" << hwnd << "  标题：" << title << endl;
	EnumChildWindows(hwnd, EnumChildWindowsProc, NULL);
	return true;
}

BOOL CALLBACK EnumThreadWindowsProc(HWND hwnd, LPARAM lParam)
{
	char title[100] = { 0 };
	GetWindowTextA(hwnd, title, 100);
	cout << hex << "句柄：" << hwnd << "  标题：" << title << endl;
	return true;
}

BOOL myEnumWindow(HWND hwnd)
{
	char title[100] = { 0 };
	HWND after = NULL;
	while (after = ::FindWindowEx(hwnd, after, NULL, NULL))
	{
		GetWindowTextA(after, title, 100);
		cout << "句柄：" << hex << after << "  标题：" << title << endl;
		myEnumWindow(after);
	}
	return true;
}

UINT ThreadProc(LPVOID lpVoid)
{
	MessageBoxA(NULL, "请选择确定还是取消？", "TestWindow2", MB_OKCANCEL);
	return 0;
}

int main()
{
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = GetWindow" << std::endl;
		std::wcout << L"输入2 = EnumWindows" << std::endl;
		std::wcout << L"输入3 = EnumChildWindows" << std::endl;
		std::wcout << L"输入4 = EnumThreadWindows" << std::endl;
		std::wcout << L"输入5 = FindWindowEx" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			HWND nHwnd = ::GetWindow(GetDesktopWindow(), GW_CHILD);
			while (nHwnd != NULL) {
				char title[100] = { 0 };
				GetWindowTextA(nHwnd, title, 100);
				cout << "句柄：" << hex << nHwnd << "  标题：" << title << endl;
				nHwnd = ::GetWindow(nHwnd, GW_HWNDNEXT);
			}
		}
		else if (iInput == 2)
		{
			BOOL bResult1 = ::EnumWindows(EnumWindowsProc, NULL);
		}
		else if (iInput == 3)
		{
			BOOL bResult2 = ::EnumChildWindows(GetDesktopWindow(), EnumChildWindowsProc, NULL);
		}
		else if (iInput == 4)
		{
			DWORD dwThreadId = 0;
			::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, NULL, 0, &dwThreadId);
			Sleep(20);
			BOOL bResult = EnumThreadWindows(dwThreadId, EnumThreadWindowsProc, NULL);
		}
		else if (iInput == 5)
		{
			myEnumWindow(NULL);
		}
	}
	return 0;
}