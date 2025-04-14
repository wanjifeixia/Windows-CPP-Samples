// LayeredWindow.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "LayeredWindow.h"

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
    LoadStringW(hInstance, IDC_LAYEREDWINDOW, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAYEREDWINDOW));

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAYEREDWINDOW));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_LAYEREDWINDOW);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//设置透明属性窗口
void MySetLayeredWindowAttributes(HWND hWnd)
{
	//设置透明属性窗口
	//SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hWnd, RGB(255, 255, 254), 100, LWA_COLORKEY | LWA_ALPHA);
}

//创建一个圆形区域
void MySetWindowRgn(HWND hWnd)
{
	HRGN hRgn = CreateEllipticRgn(0, 0, 400, 400);
	SetWindowRgn(hWnd, hRgn, TRUE);
}

//创建一个圆形区域
void MyUpdateLayeredWindow(HWND hWnd)
{

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

	//如果不使用WS_EX_LAYERED参数 则需要额外
   //SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
   HWND hWnd = CreateWindowEx(WS_EX_LAYERED, szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
      return FALSE;

   MySetLayeredWindowAttributes(hWnd);
   //MySetWindowRgn(hWnd);
   //MyUpdateLayeredWindow(hWnd);

   BYTE bAlpha = 255;
   BOOL bResult = GetLayeredWindowAttributes((HWND)hWnd, NULL, &bAlpha, NULL);

   FreeLibrary(hInst);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   auto a = SendMessage(hWnd, WM_COPYDATA, 0, 0);

   return TRUE;
}

bool IsWindowTransparent(HWND hWnd) {
	// 检查窗口是否可见
	if (!IsWindowVisible(hWnd)) {
		return false;
	}

	// 步骤1：检查窗口是否具有WS_EX_LAYERED扩展样式
	LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
	if (!(exStyle & WS_EX_LAYERED)) {
		return false; // 不是分层窗口，直接返回不透明
	}

	// 步骤2：获取分层窗口的Alpha值或颜色键
	BYTE alpha;
	DWORD flags;
	COLORREF colorKey;
	if (GetLayeredWindowAttributes(hWnd, &colorKey, &alpha, &flags)) {
		// 检查是否有Alpha通道或颜色键透明
		if ((flags & LWA_ALPHA) && alpha < 255) {
			return true; // 存在Alpha透明度
		}
		if ((flags & LWA_COLORKEY)) {
			return true; // 使用颜色键透明
		}
	}

	// 步骤3：检查窗口是否设置了透明区域（Region）
	HRGN hRgn = CreateRectRgn(0, 0, 0, 0);
	if (GetWindowRgn(hWnd, hRgn) != ERROR) {
		RECT rcWindow;
		GetWindowRect(hWnd, &rcWindow);
		// 创建全窗口区域进行比较
		HRGN hFullRgn = CreateRectRgnIndirect(&rcWindow);
		// 比较实际区域与完整区域是否一致
		if (hRgn && hFullRgn) {
			if (!EqualRgn(hRgn, hFullRgn)) {
				DeleteObject(hRgn);
				DeleteObject(hFullRgn);
				return true; // 区域有裁剪，可能存在透明部分
			}
		}
		DeleteObject(hRgn);
		DeleteObject(hFullRgn);
	}

	return false;
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
            case IDM_ABOUT:
                if (IsWindowTransparent((HWND)0x60F22))
				    MessageBox(0, L"透明窗口", 0, 0);
				else
				    MessageBox(0, L"非透明窗口", 0, 0);
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
	case WM_COPYDATA:
	    {
		    return 111;
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
