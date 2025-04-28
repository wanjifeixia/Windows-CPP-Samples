// TestDoDragDrop.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "framework.h"
#include "TestDoDragDrop.h"
#include <iostream>
#include <fstream>

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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TESTDODRAGDROP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTDODRAGDROP));

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTDODRAGDROP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TESTDODRAGDROP);
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

// See http://blogs.msdn.com/b/oldnewthing/archive/tags/what+a+drag/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <Shlobj.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

// From http://blogs.msdn.com/b/oldnewthing/archive/2004/09/20/231739.aspx

HRESULT GetUIObjectOfFile(HWND hwnd, LPCWSTR pszPath, REFIID riid, void** ppv)
{
	*ppv = NULL;
	HRESULT hr;
	LPITEMIDLIST pidl;
	SFGAOF sfgao;
	if (SUCCEEDED(hr = SHParseDisplayName(pszPath, NULL, &pidl, 0, &sfgao))) {
		IShellFolder* psf;
		LPCITEMIDLIST pidlChild;
		if (SUCCEEDED(hr = SHBindToParent(pidl, IID_IShellFolder,
			(void**)&psf, &pidlChild))) {
			hr = psf->GetUIObjectOf(hwnd, 1, &pidlChild, riid, NULL, ppv);
			psf->Release();
		}
		CoTaskMemFree(pidl);
	}
	return hr;
}

// ==== 

struct sFindWindowWildcard
{
	const _TCHAR* lpClassName;
	const _TCHAR* lpWindowName;
	HWND hwnd;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	if (hwnd != GetConsoleWindow())
	{
		sFindWindowWildcard* fww = (sFindWindowWildcard*)lParam;

		TCHAR name[1024];
		TCHAR wndclass[1024];
		GetWindowText(hwnd, name, 1024);
		GetClassName(hwnd, wndclass, 1024);

		if ((fww->lpClassName == NULL || PathMatchSpec(wndclass, fww->lpClassName))
			&& (fww->lpWindowName == NULL || PathMatchSpec(name, fww->lpWindowName)))
		{
			fww->hwnd = hwnd;
			return FALSE;
		}
		else
			return TRUE;
	}
	else
		return TRUE;
}

HWND FindWindowWildcard(const _TCHAR* lpClassName, const _TCHAR* lpWindowName)
{
	//return FindWindow(lpClassName, lpWindowName);

	sFindWindowWildcard fww;
	fww.lpClassName = lpClassName;
	fww.lpWindowName = lpWindowName;
	fww.hwnd = NULL;

	if (lpClassName != NULL || lpWindowName != NULL)
		EnumWindows(EnumWindowsProc, (LPARAM)&fww);

	return fww.hwnd;
}

void DoDragDrop(IDropTarget* pdt, const std::vector<std::wstring>& files)
{
	IDataObject* pdtobj = NULL;
	for (std::vector<std::wstring>::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		// TODO Dropping one at a time. Should lookup how drop all as one.
		if (SUCCEEDED(GetUIObjectOfFile(NULL, it->c_str(), IID_IDataObject, (void**)&pdtobj))) {
			POINTL pt = { 0, 0 };
			DWORD dwEffect = DROPEFFECT_COPY | DROPEFFECT_LINK;

			if (SUCCEEDED(pdt->DragEnter(pdtobj, MK_LBUTTON, pt, &dwEffect))) {
				dwEffect &= DROPEFFECT_COPY | DROPEFFECT_LINK;
				if (dwEffect) {
					pdt->Drop(pdtobj, MK_LBUTTON, pt, &dwEffect);
				}
				else {
					pdt->DragLeave();
				}
			}
			pdtobj->Release();
		}
	}
}

void DoDragDrop(HWND hWndDest, const std::vector<std::wstring>& files)
{
	size_t size = sizeof(DROPFILES) + sizeof(_TCHAR);
	for (std::vector<std::wstring>::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		size += (it->length() + 1) * sizeof(_TCHAR);
	}

	HGLOBAL hGlobal = GlobalAlloc(GHND, size);
	DROPFILES* df = static_cast<DROPFILES*>(GlobalLock(hGlobal));
	df->pFiles = sizeof(DROPFILES);
	_TCHAR* f = reinterpret_cast<_TCHAR*>(df + 1);
	size -= sizeof(DROPFILES) + sizeof(_TCHAR);
	for (std::vector<std::wstring>::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		_tcscpy_s(f, it->length() + 1, it->c_str());
		size -= (it->length() + 1) * sizeof(_TCHAR);
		f += it->length() + 1;
	}
	df->fNC = TRUE;
	df->fWide = TRUE;
	GlobalUnlock(hGlobal);

	PostMessage(hWndDest, WM_DROPFILES, (WPARAM)hGlobal, 0);
	GlobalFree(hGlobal);
}

// Function to copy files to clipboard using CF_HDROP format
bool CopyFilesToClipboard(const std::vector<std::wstring>& files) {
	if (!OpenClipboard(NULL)) {
		std::cerr << "Failed to open clipboard!" << std::endl;
		return false;
	}

	EmptyClipboard();

	// Calculate the size of the DROPFILES structure and the file paths
	size_t size = sizeof(DROPFILES);
	for (const auto& file : files) {
		size += (file.size() + 1) * sizeof(wchar_t); // File name size + NULL terminator
	}
	size += sizeof(wchar_t); // Additional NULL terminator at the end

	// Allocate memory for the DROPFILES structure
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!hMem) {
		CloseClipboard();
		std::cerr << "Failed to allocate memory!" << std::endl;
		return false;
	}

	// Fill the DROPFILES structure
	DROPFILES* pDropFiles = (DROPFILES*)GlobalLock(hMem);
	pDropFiles->pFiles = sizeof(DROPFILES);
	pDropFiles->fNC = FALSE;
	pDropFiles->fWide = TRUE; // Unicode format

	// Copy the file paths into the allocated memory
	wchar_t* pData = (wchar_t*)((BYTE*)pDropFiles + sizeof(DROPFILES));
	for (const auto& file : files) {
		wcscpy_s(pData, file.size() + 1, file.c_str());
		pData += file.size() + 1; // Move pointer to the next file path
	}
	*pData = L'\0'; // Add the final NULL terminator

	GlobalUnlock(hMem);

	// Set the data in the clipboard
	if (!SetClipboardData(CF_HDROP, hMem)) {
		GlobalFree(hMem);
		CloseClipboard();
		std::cerr << "Failed to set clipboard data!" << std::endl;
		return false;
	}

	CloseClipboard();
	return true;
}

void SimulatePaste() {
	INPUT inputs[4] = {};

	// 模拟按下 Ctrl 键
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_CONTROL;
	SendInput(1, &inputs[0], sizeof(INPUT));

	// 模拟按下 V 键
	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = 'V';
	SendInput(1, &inputs[1], sizeof(INPUT));

	// 模拟释放 V 键
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &inputs[1], sizeof(INPUT));

	// 模拟释放 Ctrl 键
	inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &inputs[0], sizeof(INPUT));
}

#include <Oleacc.h>
#pragma comment(lib, "Oleacc.lib")

// 通过窗口句柄获取IDropTarget
HRESULT GetDropTargetFromWindow(HWND hwnd, IDropTarget** ppDropTarget)
{
	if (!ppDropTarget || !hwnd)
		return E_INVALIDARG;

	*ppDropTarget = nullptr;

	// 通过窗口句柄获取 IAccessible 接口
	IAccessible* pAcc = nullptr;
	HRESULT hr = AccessibleObjectFromWindow(hwnd, OBJID_WINDOW, IID_IAccessible, (void**)&pAcc);

	if (SUCCEEDED(hr))
	{
		// 获取 IDropTarget 接口
		hr = pAcc->QueryInterface(IID_IDropTarget, (void**)ppDropTarget);
		pAcc->Release();
	}

	return hr;
}

#include <wrl/client.h>

std::wstring ExplorerViewPath()
{
	std::wstring path;
	HRESULT hr = NOERROR;

	// the top context menu in Win11 does not
	// provide an IOleWindow with the SetSite() object,
	// so we have to use a trick to get it: since the
	// context menu must always be the top window, we
	// just grab the foreground window and assume that
	// this is the explorer window.
	auto hwnd = ::GetForegroundWindow();
	if (!hwnd)
		return path;

	wchar_t szName[1024] = { 0 };
	::GetClassName(hwnd, szName, _countof(szName));
	if (StrCmp(szName, L"WorkerW") == 0 || StrCmp(szName, L"Progman") == 0)
	{
		//special folder: desktop
		hr = ::SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, szName);
		if (FAILED(hr))
			return path;

		path = szName;
		return path;
	}

	if (StrCmp(szName, L"CabinetWClass") != 0)
		return path;

	// get the shell windows object to enumerate all active explorer
	// instances. We use those to compare the foreground hwnd to it.
	Microsoft::WRL::ComPtr<IShellWindows> shell;
	if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_IShellWindows, reinterpret_cast<LPVOID*>(shell.GetAddressOf()))))
		return path;

	if (!shell)
		return path;

	Microsoft::WRL::ComPtr<IDispatch> disp;
	VARIANT variant{};
	variant.vt = VT_I4;

	Microsoft::WRL::ComPtr<IWebBrowserApp> browser;
	// look for correct explorer window
	for (variant.intVal = 0; shell->Item(variant, disp.GetAddressOf()) == S_OK; variant.intVal++)
	{
		Microsoft::WRL::ComPtr<IWebBrowserApp> tmp;
		if (FAILED(disp->QueryInterface(tmp.GetAddressOf())))
			continue;

		HWND tmpHwnd = nullptr;
		hr = tmp->get_HWND(reinterpret_cast<SHANDLE_PTR*>(&tmpHwnd));
		if (hwnd == tmpHwnd)
		{
			browser = tmp;
			break; // found it!
		}
	}

	if (browser != nullptr)
	{
		BSTR url;
		hr = browser->get_LocationURL(&url);
		if (FAILED(hr))
			return path;

		std::wstring sUrl(url, SysStringLen(url));
		SysFreeString(url);
		DWORD size = _countof(szName);
		hr = ::PathCreateFromUrl(sUrl.c_str(), szName, &size, NULL);
		if (SUCCEEDED(hr))
			path = szName;
	}

	return path;
}

void openFolderAndSelectItems(const std::wstring& folderPath, const std::vector<std::wstring>& entries) {
	LPITEMIDLIST folderID;
	HRESULT result = SHParseDisplayName(folderPath.c_str(), nullptr, &folderID, 0, nullptr);

	if (FAILED(result)) {
		std::wcerr << L"无法解析文件夹路径: " << folderPath << std::endl;
		return;
	}

	std::vector<LPITEMIDLIST> entriesID;
	entriesID.reserve(entries.size());

	for (const std::wstring& entry : entries) {
		LPITEMIDLIST entryID;
		result = SHParseDisplayName(entry.c_str(), nullptr, &entryID, 0, nullptr);

		if (SUCCEEDED(result)) {
			entriesID.emplace_back(entryID);
		}
		else {
			std::wcerr << L"无法解析条目路径: " << entry << std::endl;
		}
	}

	// 选择是打开桌面还是资源管理器
	if (folderPath == L"") {
		result = SHOpenFolderAndSelectItems(nullptr, entriesID.size(), (LPCITEMIDLIST*)entriesID.data(), OFASI_OPENDESKTOP);
	}
	else {
		result = SHOpenFolderAndSelectItems(folderID, entriesID.size(), (LPCITEMIDLIST*)entriesID.data(), 0);
	}

	// 释放 PIDL
	ILFree(folderID);
	for (LPITEMIDLIST entryID : entriesID) {
		ILFree(entryID);
	}

	if (FAILED(result)) {
		std::wcerr << L"打开文件夹并选中文件失败，错误代码: " << result << std::endl;
	}
}

// 获取当前进程所在目录的函数
std::wstring getCurrentDirectoryW() {
	wchar_t buffer[MAX_PATH];
	DWORD length = GetModuleFileNameW(NULL, buffer, MAX_PATH);
	if (length == 0) {
		throw std::runtime_error("Failed to get current directory.");
	}

	std::wstring path(buffer);
	size_t pos = path.find_last_of(L"\\/");
	if (pos != std::wstring::npos) {
		path = path.substr(0, pos); // 去掉文件名，保留目录路径
	}
	return path;
}

int DropFileToExplorer(BOOL bOpenSingleFile)
{
	//方案一 Hook资源管理器，然后执行DropTarget接口
	if (0)
	{
		HMODULE hDll = LoadLibrary(L"DoDragDropDll.dll");
		if (hDll == NULL)
			return FALSE;

		typedef BOOL(WINAPI* pInstallHook)();
		typedef void(WINAPI* pUninstallHook)();
		pInstallHook InstallHook = (pInstallHook)GetProcAddress(hDll, "InstallHook");
		if (InstallHook == NULL || !InstallHook())
		{
			return FALSE;
		}
		return 1;
	}

	//方案二 将文件写入剪切板，然后粘贴到资源管理器
	if (0)
	{
		std::vector<std::wstring> files = { L"C:\\Users\\yixian\\Desktop\\PEview.exe" };

		// Step 1: Copy files to clipboard
		if (!CopyFilesToClipboard(files)) {
			std::cerr << "Failed to copy files to clipboard!" << std::endl;
			return 1;
		}

		// Step 2: Find the target window by its title (e.g., File Explorer window)
		// 确保资源管理器窗口获得焦点
		HWND hwndTarget = FindWindow(nullptr, L"222222 - 文件资源管理器"); // 或者其他窗口标题
		if (hwndTarget) {
			SetForegroundWindow(hwndTarget);
			Sleep(200); // 等待 0.5 秒以确保窗口获得焦点
			SimulatePaste();
			std::cout << "模拟粘贴操作已完成!" << std::endl;
		}
		else {
			std::cerr << "未找到资源管理器窗口!" << std::endl;
		}

		std::cout << "Files successfully pasted to target window!" << std::endl;
		return 0;
	}

	//方案三 获取资源管理器的真实路径，然后复制文件
	if (0)
	{
		HWND hwndTarget = FindWindow(L"CabinetWClass", NULL);
		if (hwndTarget)
			SetForegroundWindow(hwndTarget);

		auto path = ExplorerViewPath();
		return 0;
	}

	//选中桌面或资源管理器文件
	if (1)
	{
		std::wstring currentDir = getCurrentDirectoryW();

		std::wstring filePath = currentDir + L"\\TestFile0001.txt";
		std::wofstream outFile(filePath);
		if (outFile.is_open())
			outFile.close();
		std::vector<std::wstring> fileNames;
		fileNames.push_back(filePath);
		
		//支持多个文件
		if (!bOpenSingleFile)
		{
			std::wstring filePath2 = currentDir + L"\\TestFile0002.txt";
			std::wofstream outFile2(filePath2);
			if (outFile2.is_open())
				outFile2.close();
			fileNames.push_back(filePath2);
		}

		//选中桌面文件
		//openFolderAndSelectItems(L"", fileNames);
		//选中资源管理器文件
		CoInitialize(0);
		openFolderAndSelectItems(currentDir, fileNames);
		CoUninitialize();
		return 0;
	}

	//不可行 因为无法跨进程调用IDropTarget
	if (0)
	{
		CoInitialize(0);

		std::vector<std::wstring> files;
		files.push_back(L"C:\\Users\\yixian\\Desktop\\W11ClassicMenu.zip");
		HWND hWndDest = FindWindow(nullptr, L"22222 - 文件资源管理器");
		if (hWndDest != NULL)
		{
			LONG ExStyle = GetWindowLong(hWndDest, GWL_EXSTYLE);

			IDropTarget* pdt = (IDropTarget*)GetProp(hWndDest, _T("OleDropTargetInterface"));
			//if (SUCCEEDED(GetUIObjectOfFile(hWndDest, _T("C:\\Windows\\explorer.exe"), IID_IDropTarget, (void**)&pdt))) {}

			if (pdt != NULL)
			{
				DoDragDrop(pdt, files);
			}
			else if ((ExStyle & WS_EX_ACCEPTFILES) == WS_EX_ACCEPTFILES)
			{
				DoDragDrop(hWndDest, files);
			}
			else
			{
				_tprintf(_T("Window 0x%x doesnt support drag and drop.\n"), HandleToUlong(hWndDest));
			}
		}
		else
		{
			_tprintf(_T("Could not find window with class and name.\n"));
		}

		CoUninitialize();
	}
	return 0;
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
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case ID_OPEN_ONE_FILE:
				DropFileToExplorer(TRUE);
                break;
			case ID_OPEN_MORE_FILE:
				DropFileToExplorer(FALSE);
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
		{
			std::wstring currentDir = getCurrentDirectoryW();
			std::wstring filePath = currentDir + L"\\TestFile0001.txt";
			DeleteFile(filePath.c_str());
			filePath = currentDir + L"\\TestFile0002.txt";
			DeleteFile(filePath.c_str());
			PostQuitMessage(0);
		}
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
