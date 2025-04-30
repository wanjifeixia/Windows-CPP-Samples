// CaptureAnImage.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
#include <iostream>

using namespace std;

HHOOK g_hHook = NULL;
LRESULT CALLBACK LowLevelKeyboardProc(
	int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* Key_Info = (KBDLLHOOKSTRUCT*)lParam;

	if (HC_ACTION == nCode)
	{
		if (WM_KEYDOWN == wParam || WM_SYSKEYDOWN == wParam)
		{
			//禁用截图按键
			if (Key_Info->vkCode == VK_SNAPSHOT)
			{
				wcout << L"VK_SNAPSHOT截图功能已失效" << endl;
				return TRUE;
			}
		}
	}

	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

BOOL SetHookOn()
{
	if (g_hHook != NULL)
	{
		return FALSE;
	}
	g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, NULL);
	if (NULL == g_hHook)
	{
		MessageBox(NULL, L"安装钩子出错 !", L"error", MB_ICONSTOP);
		return FALSE;
	}

	return TRUE;
}

BOOL SetHookOff()
{
	if (g_hHook == NULL)
	{
		return FALSE;
	}
	UnhookWindowsHookEx(g_hHook);
	g_hHook = NULL;
	return TRUE;
}

int main()
{
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));
	std::wcout.imbue(std::locale(""));

	wcout << L"禁用VK_SNAPSHOT截图功能" << endl;
	SetHookOn();

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}