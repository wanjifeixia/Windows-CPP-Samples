// WindowsExplorerMenu.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "WindowsExplorerMenu.h"
#include <fstream>
#include <iostream>
#include <ShObjIdl.h>
#include <atlcomcli.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <commoncontrols.h>
#include <vector>
#include "ShellContextMenu.h"
#include "ShellMenu.h"

using namespace std;

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain_NewFile(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCmdLine, int iCmdShow);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。
 	CoInitialize(NULL);

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSEXPLORERMENU, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSEXPLORERMENU));

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSEXPLORERMENU));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINDOWSEXPLORERMENU);
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

static IContextMenu2* g_pIContext2 = NULL;
static IContextMenu3* g_pIContext3 = NULL;
HICON GetFileIcon(LPCSTR filePath, int iImageList, int chicun);
INT RightMenu(HWND handle);
INT RightMenu(HWND handle, LPCSTR folder);

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
    switch (message)
    {
// 	case WM_MENUCHAR:
// 		if (g_pIContext3)
// 		{
// 			LRESULT lResult = 0;
// 			g_pIContext3->HandleMenuMsg2(message, wParam, lParam, &lResult);
// 			return(lResult);
// 		}
// 		break;
// 	case WM_DRAWITEM:
// 	case WM_MEASUREITEM:
// 		if (wParam)
// 		{
// 			break;
// 		}
// 	case WM_INITMENUPOPUP:
// 		if (g_pIContext2)
// 		{
// 			g_pIContext2->HandleMenuMsg(message, wParam, lParam);
// 		}
// 		else
// 		{
// 			if (g_pIContext3)
// 				g_pIContext3->HandleMenuMsg(message, wParam, lParam);
// 		}
// 		return(message == WM_INITMENUPOPUP ? 0 : TRUE);
// 		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 分析菜单选择:
		switch (wmId)
		{
		case IDM_ABOUT:
		{
			//------------------------------文件右键菜单-----------------------------
			
			//方案一
			if (0)
			{
				RightMenu(hWnd, "C:\\Users\\46043\\Desktop\\新建 文本文档.txt");
			}

			//方案二
			if (0)
			{
				ShellContextMenu shellMenu;
				shellMenu.setPath(L"C:\\Users\\46043\\Desktop\\新建 文本文档.txt");
				POINT pt;
				GetCursorPos(&pt);
				shellMenu.showContextMenu(hWnd, pt);
			}

			//方案三
			if (1)
			{
				CShellMenu ShellMenu(hWnd, (TCHAR*)L"C:\\Users\\46043\\Desktop\\新建 文本文档.txt", FALSE);
				if (ShellMenu.IsCreationSuccessful())
				{
					// get mouse position
					POINT Point;
					::GetCursorPos(&Point);

					// display shell menu
					if (ShellMenu.Show(Point.x, Point.y))
					{
						// if user has done an action on shell menu,
						// close current menu
						EndMenu();
					}
				}
			}
		}
		break;
		case IDM_EXIT:
		{
			//------------------------------桌面右键菜单-----------------------------

			//方案一
			if (0)
			{
				RightMenu(hWnd);
			}

			//方案二
			if (1)
			{
				CShellMenu ShellMenu(hWnd, CSIDL_DESKTOP, TRUE);
				if (ShellMenu.IsCreationSuccessful())
				{
					// get mouse position
					POINT Point;
					::GetCursorPos(&Point);

					// display shell menu
					if (ShellMenu.Show(Point.x, Point.y))
					{
						// if user has done an action on shell menu,
						// close current menu
						EndMenu();
					}
				}
			}
		}
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

//右击桌面空白处
INT RightMenu(HWND hWnd)
{
	LPCONTEXTMENU pContextMenu = NULL;
	LPCONTEXTMENU pCtxMenuTemp = NULL;
	g_pIContext2 = NULL;
	g_pIContext3 = NULL;
	int menuType = 0;

	CComQIPtr<IShellFolder> pFolder;
	if (FAILED(SHGetDesktopFolder(&pFolder)))
		return -1;

// 	PIDLIST_ABSOLUTE pidl;
// 	SHParseDisplayName(L"E:\\Users\\Enlink\\Desktop\\1111", NULL, &pidl, 0, NULL);
// 	PCUITEMID_CHILD child;
// 	CComQIPtr<IShellFolder> pFolder;
// 	SHBindToParent(pidl, IID_IShellFolder, (void**)&pFolder, &child);

	CComQIPtr<IShellView> pShellView;
	if (FAILED(pFolder->CreateViewObject(hWnd, IID_PPV_ARGS(&pShellView))))
		return -1;

	HRESULT hRslt = pShellView->GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu3, (LPVOID*)&pCtxMenuTemp);
	if (FAILED(hRslt))
		return -1;

	POINT pt;
	GetCursorPos(&pt);
	if (pCtxMenuTemp->QueryInterface(IID_IContextMenu3, (void**)&pContextMenu) == NOERROR)
	{
		menuType = 3;

		std::cout << std::endl << "menu type is 3" << std::endl;
	}
	else if (pCtxMenuTemp->QueryInterface(IID_IContextMenu2, (void**)&pContextMenu) == NOERROR)
	{
		menuType = 2;

		std::cout << std::endl << "menu type is 2" << std::endl;
	}
	if (pContextMenu)
	{
		std::cout << "new menu got" << std::endl;
		pCtxMenuTemp->Release();
	}
	else
	{
		std::cout << "it's old menu" << std::endl;
		pContextMenu = pCtxMenuTemp;
		menuType = 1;
	}
	if (menuType == 0)
	{
		std::cout << "there's no menu" << std::endl;
		return -1;
	}
	HMENU hMenu = CreatePopupMenu();
	hRslt = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_EXPLORE);

	if (FAILED(hRslt))
	{
		return -1;
	}
	WNDPROC oldWndProc = NULL;
	if (menuType > 1)
	{
		DWORD err = GetLastError();
		if (err)
			std::cout << "Something wrong when SetWidowLongPtr: " << err << std::endl;
		if (menuType == 2)
		{
			g_pIContext2 = (LPCONTEXTMENU2)pContextMenu;
		}
		else
		{
			g_pIContext3 = (LPCONTEXTMENU3)pContextMenu;
		}
	}
	else
	{
		oldWndProc = NULL;
	}
	int cmd = TrackPopupMenu(
		hMenu,
		TPM_RETURNCMD | TPM_LEFTBUTTON,
		pt.x,
		pt.y,
		0,
		hWnd,
		0);
	if (oldWndProc)
	{
		SetWindowLongPtr(hWnd, -4, (LONG)oldWndProc);
	}
	if (cmd != 0)
	{
		CMINVOKECOMMANDINFO ci = { 0 };
		ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
		ci.hwnd = hWnd;
		ci.lpVerb = (LPCSTR)MAKEINTRESOURCE(cmd - 1);
		ci.nShow = SW_SHOWNORMAL;
		pContextMenu->InvokeCommand(&ci);
	}
	pContextMenu->Release();
	g_pIContext2 = NULL;
	g_pIContext3 = NULL;
	DestroyMenu(hMenu);
	return cmd;
}

//右击文件或者文件夹
INT RightMenu(HWND handle, LPCSTR folder)
{
	USES_CONVERSION;
	char* LineChar = (char*)folder;
	const WCHAR* cLineChar = A2W(LineChar);

	PIDLIST_ABSOLUTE pidl;
	SHParseDisplayName(cLineChar, NULL, &pidl, 0, NULL);
	PCUITEMID_CHILD child;
	CComQIPtr<IShellFolder> pFolder;
	SHBindToParent(pidl, IID_IShellFolder, (void**)&pFolder, &child);

	HWND hwnd = handle;
	LPCONTEXTMENU pContextMenu = NULL;
	LPCONTEXTMENU pCtxMenuTemp = NULL;
	g_pIContext2 = NULL;
	g_pIContext3 = NULL;
	int menuType = 0;

	HRESULT hRslt = pFolder->GetUIObjectOf(
		0,
		1,
		&child,
		IID_IContextMenu,
		0,
		(void**)&pCtxMenuTemp);
	if (FAILED(hRslt))
	{
		return -1;
	}

	// 	IShellItem* psi;
	// 	hRslt = SHCreateItemFromParsingName(L"C:\\Users\\yixian\\Desktop\\新建 文本文档.txt", NULL, IID_IShellItem, (void**)&psi);
	// 	// 获取上下文菜单
	// 	psi->BindToHandler(NULL, BHID_SFUIObject, IID_IContextMenu, (void**)&pCtxMenuTemp);

	POINT pt;
	GetCursorPos(&pt);
	if (pCtxMenuTemp->QueryInterface(IID_IContextMenu3, (void**)&pContextMenu) == NOERROR)
	{
		menuType = 3;

		std::cout << std::endl << "menu type is 3" << std::endl;
	}
	else if (pCtxMenuTemp->QueryInterface(IID_IContextMenu2, (void**)&pContextMenu) == NOERROR)
	{
		menuType = 2;

		std::cout << std::endl << "menu type is 2" << std::endl;
	}
	if (pContextMenu)
	{
		std::cout << "new menu got" << std::endl;
		pCtxMenuTemp->Release();
	}
	else
	{
		std::cout << "it's old menu" << std::endl;
		pContextMenu = pCtxMenuTemp;
		menuType = 1;
	}
	if (menuType == 0)
	{
		std::cout << "there's no menu" << std::endl;
		return -1;
	}
	HMENU hMenu = CreatePopupMenu();
	hRslt = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_EXPLORE);

	if (FAILED(hRslt))
	{
		return -1;
	}
	WNDPROC oldWndProc = NULL;
	if (menuType > 1)
	{
		DWORD err = GetLastError();
		if (err)
			std::cout << "Something wrong when SetWidowLongPtr: " << err << std::endl;
		if (menuType == 2)
		{
			g_pIContext2 = (LPCONTEXTMENU2)pContextMenu;
		}
		else
		{
			g_pIContext3 = (LPCONTEXTMENU3)pContextMenu;
		}
	}
	else
	{
		oldWndProc = NULL;
	}
	int cmd = TrackPopupMenu(
		hMenu,
		TPM_RETURNCMD | TPM_LEFTBUTTON,
		pt.x,
		pt.y,
		0,
		hwnd,
		0);
	if (oldWndProc)
	{
		SetWindowLongPtr(handle, -4, (LONG)oldWndProc);
	}
	if (cmd != 0)
	{
		CMINVOKECOMMANDINFO ci = { 0 };
		ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
		ci.hwnd = hwnd;
		ci.lpVerb = (LPCSTR)MAKEINTRESOURCE(cmd - 1);
		ci.nShow = SW_SHOWNORMAL;
		pContextMenu->InvokeCommand(&ci);
	}
	pContextMenu->Release();
	g_pIContext2 = NULL;
	g_pIContext3 = NULL;
	DestroyMenu(hMenu);
	return cmd;
}

//原始：右击文件或者文件夹
INT RightMenu_bak(HWND handle, LPCSTR folder)
{
	USES_CONVERSION;
	char* LineChar = (char*)folder;
	const WCHAR* cLineChar = A2W(LineChar);

	PIDLIST_ABSOLUTE pidl;
	SHParseDisplayName(cLineChar, NULL, &pidl, 0, NULL);
	PCUITEMID_CHILD child;
	CComQIPtr<IShellFolder> pFolder;
	SHBindToParent(pidl, IID_IShellFolder, (void**)&pFolder, &child);

	HWND hwnd = handle;
	LPCONTEXTMENU pContextMenu = NULL;
	LPCONTEXTMENU pCtxMenuTemp = NULL;
	g_pIContext2 = NULL;
	g_pIContext3 = NULL;
	int menuType = 0;

	HRESULT hRslt = pFolder->GetUIObjectOf(
		0,
		1,
		&child,
		IID_IContextMenu,
		0,
		(void**)&pCtxMenuTemp);
	if (FAILED(hRslt))
	{
		return -1;
	}

// 	IShellItem* psi;
// 	hRslt = SHCreateItemFromParsingName(L"C:\\Users\\yixian\\Desktop\\新建 文本文档.txt", NULL, IID_IShellItem, (void**)&psi);
// 	// 获取上下文菜单
// 	psi->BindToHandler(NULL, BHID_SFUIObject, IID_IContextMenu, (void**)&pCtxMenuTemp);

	POINT pt;
	GetCursorPos(&pt);
	if (pCtxMenuTemp->QueryInterface(IID_IContextMenu3, (void**)&pContextMenu) == NOERROR)
	{
		menuType = 3;

		std::cout << std::endl << "menu type is 3" << std::endl;
	}
	else if (pCtxMenuTemp->QueryInterface(IID_IContextMenu2, (void**)&pContextMenu) == NOERROR)
	{
		menuType = 2;

		std::cout << std::endl << "menu type is 2" << std::endl;
	}
	if (pContextMenu)
	{
		std::cout << "new menu got" << std::endl;
		pCtxMenuTemp->Release();
	}
	else
	{
		std::cout << "it's old menu" << std::endl;
		pContextMenu = pCtxMenuTemp;
		menuType = 1;
	}
	if (menuType == 0)
	{
		std::cout << "there's no menu" << std::endl;
		return -1;
	}
	HMENU hMenu = CreatePopupMenu();
	hRslt = pContextMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_EXPLORE);

	if (FAILED(hRslt))
	{
		return -1;
	}
	WNDPROC oldWndProc = NULL;
	if (menuType > 1)
	{
		DWORD err = GetLastError();
		if (err)
			std::cout << "Something wrong when SetWidowLongPtr: " << err << std::endl;
		if (menuType == 2)
		{
			g_pIContext2 = (LPCONTEXTMENU2)pContextMenu;
		}
		else
		{
			g_pIContext3 = (LPCONTEXTMENU3)pContextMenu;
		}
	}
	else
	{
		oldWndProc = NULL;
	}
	int cmd = TrackPopupMenu(
		hMenu,
		TPM_RETURNCMD | TPM_LEFTBUTTON,
		pt.x,
		pt.y,
		0,
		hwnd,
		0);
	if (oldWndProc)
	{
		SetWindowLongPtr(handle, -4, (LONG)oldWndProc);
	}
	if (cmd != 0)
	{
		CMINVOKECOMMANDINFO ci = { 0 };
		ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
		ci.hwnd = hwnd;
		ci.lpVerb = (LPCSTR)MAKEINTRESOURCE(cmd - 1);
		ci.nShow = SW_SHOWNORMAL;
		pContextMenu->InvokeCommand(&ci);
	}
	pContextMenu->Release();
	g_pIContext2 = NULL;
	g_pIContext3 = NULL;
	DestroyMenu(hMenu);
	return cmd;
}

HICON GetFileIcon(LPCSTR filePath, int iImageList, int chicun)
{
	//iImageList = 2;
	SHFILEINFOA sfi = { 0 };
	SHGetFileInfoA(filePath, -1, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);
	IImageList* imageList = NULL;
	SHGetImageList(iImageList, IID_IImageList, (void**)&imageList);
	HICON hIcon = NULL;
	imageList->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hIcon);
	imageList->Release();

	return hIcon;
}