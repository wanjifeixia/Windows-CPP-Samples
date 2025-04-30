// TestSetOverlayIcon.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "TestSetOverlayIcon.h"
#include <ShObjIdl.h>

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HICON g_hGreenIcon = NULL;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。
	g_hGreenIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_GREEN));

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TESTSETOVERLAYICON, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTSETOVERLAYICON));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTSETOVERLAYICON));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TESTSETOVERLAYICON);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//打开任务栏徽章(角标)开关
void OpenTaskbarBadges()
{
	HKEY hKey = NULL;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_ALL_ACCESS, &hKey) == 0)
	{
		DWORD dwData = 0;
		DWORD dataSize = sizeof(DWORD);
		DWORD dwType = REG_DWORD;
		RegQueryValueExW(hKey, L"TaskbarBadges", nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwData), &dataSize);

		if (dwData != 1)
		{
			DWORD dwValue = 1;
			RegSetValueEx(hKey, L"TaskbarBadges", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

			//立即刷新系统
			DWORD dwResult;
			SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, NULL, NULL, SMTO_ABORTIFHUNG, 100, (PDWORD_PTR)&dwResult);
		}
		RegCloseKey(hKey);
	}
}

//添加角标
void SetOverlayIcon(HWND hWnd, HICON hIcon)
{
	if (hWnd == NULL)
		return;

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
		return;

	ITaskbarList3* pTaskbarInterface = NULL;
	if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
		IID_ITaskbarList3, reinterpret_cast<void**> (&(pTaskbarInterface)))))
	{
		if (SUCCEEDED(pTaskbarInterface->HrInit()))
		{
			pTaskbarInterface->SetOverlayIcon(hWnd, hIcon, L"");
		}
		pTaskbarInterface->Release();
	}
	CoUninitialize();
}

//添加或删除任务栏
void ShowInTaskbar(HWND hWnd, BOOL bShow)
{
	HRESULT hr;
	ITaskbarList* pTaskbarList;
	hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList, (void**)&pTaskbarList);

	if (SUCCEEDED(hr))
	{
		pTaskbarList->HrInit();
		if (bShow)
			pTaskbarList->AddTab(hWnd);
		else
			pTaskbarList->DeleteTab(hWnd);

		pTaskbarList->Release();
	}
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //创建任务栏的消息
	static UINT s_uTBBC = WM_NULL;
	if (s_uTBBC == WM_NULL)
	{
		// Compute the value for the TaskbarButtonCreated message
		s_uTBBC = RegisterWindowMessage(L"TaskbarButtonCreated");

		// In case the application is run elevated, allow the
		// TaskbarButtonCreated message through.
		ChangeWindowMessageFilter(s_uTBBC, MSGFLT_ADD);
	}

	if (message == s_uTBBC)
	{
		SetOverlayIcon(hWnd, g_hGreenIcon);
	}

    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_OpenTaskbarBadges:
				OpenTaskbarBadges();
                break;
			case ID_SetOverlayIcon:
				SetOverlayIcon(hWnd, g_hGreenIcon);
				break;
			case ID_ClearOverlayIcon:
				SetOverlayIcon(hWnd, NULL);
				break;
			case ID_SetOverlayIcon_CMD:
            {
				HWND hWnd = FindWindow(L"ConsoleWindowClass", NULL);
				if (!hWnd)
                    hWnd = FindWindow(NULL, L"命令提示符");
				SetOverlayIcon(hWnd, g_hGreenIcon);
				break;
            }
			case ID_AddTab:
                ShowInTaskbar(hWnd, TRUE);
				break;
			case ID_DeleteTab:
                ShowInTaskbar(hWnd, FALSE);
				break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
