// SetWallPaper.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "SetWallPaper.h"
#include <iostream>
#include <wininet.h>
#include <shlobj.h>
#include <string>
#include <iostream>

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

enum WallpaperStyle
{
	Tile,//平铺
	Center,//居中
	Stretch,//拉伸
	Fit, //适应
	Fill//填充
};

//
//   FUNCTION: SetDesktopWallpaper(PCWSTR, WallpaperStyle)
//
//   PURPOSE: Set the desktop wallpaper.
//
//   PARAMETERS: 
//   * pszFile - Path of the wallpaper
//   * style - Wallpaper style
//
HRESULT SetDesktopWallpaper(LPCWSTR pszFile, WallpaperStyle style)
{
	HRESULT hr = S_OK;

	//设置壁纸风格和展开方式
	//在Control Panel\Desktop中的两个键值将被设置
	// TileWallpaper
	//  0: 图片不被平铺 
	//  1: 被平铺 
	// WallpaperStyle
	//  0:  0表示图片居中，1表示平铺
	//  2:  拉伸填充整个屏幕
	//  6:  拉伸适应屏幕并保持高度比
	//  10: 图片被调整大小裁剪适应屏幕保持纵横比

	//以可读可写的方式打开HKCU\Control Panel\Desktop注册表项
	HKEY hKey = NULL;
	hr = HRESULT_FROM_WIN32(RegOpenKeyEx(HKEY_CURRENT_USER,
		L"Control Panel\\Desktop", 0, KEY_READ | KEY_WRITE, &hKey));
	if (SUCCEEDED(hr))
	{
		LPCWSTR pszWallpaperStyle = NULL;
		LPCWSTR pszTileWallpaper = NULL;

		switch (style)
		{
		case Tile:
			pszWallpaperStyle = L"0";
			pszTileWallpaper = L"1";
			break;

		case Center:
			pszWallpaperStyle = L"0";
			pszTileWallpaper = L"0";
			break;

		case Stretch:
			pszWallpaperStyle = L"2";
			pszTileWallpaper = L"0";
			break;

		case Fit: // (Windows 7 and later)
			pszWallpaperStyle = L"6";
			pszTileWallpaper = L"0";
			break;

		case Fill: // (Windows 7 and later)
			pszWallpaperStyle = L"10";
			pszTileWallpaper = L"0";
			break;
		}
		// 设置 WallpaperStyle 和 TileWallpaper 到注册表项.
		DWORD cbData = lstrlen(pszWallpaperStyle) * sizeof(*pszWallpaperStyle);
		hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, L"WallpaperStyle", 0, REG_SZ,
			reinterpret_cast<const BYTE*>(pszWallpaperStyle), cbData));
		if (SUCCEEDED(hr))
		{
			cbData = lstrlen(pszTileWallpaper) * sizeof(*pszTileWallpaper);
			hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, L"TileWallpaper", 0, REG_SZ,
				reinterpret_cast<const BYTE*>(pszTileWallpaper), cbData));
		}

		RegCloseKey(hKey);
	}

	//通过调用Win32 API函数SystemParametersInfo 设置桌面壁纸
	/************************************************
	之前我们已经设置了壁纸的类型，但是壁纸图片的实际文件路径还没
	设置。SystemParametersInfo 这个函数位于user32.dll中，它支持
	我们从系统中获得硬件和配置信息。它有四个参数，第一个指明调用这
	个函数所要执行的操作，接下来的两个参数指明将要设置的数据，依据
	第一个参数的设定。最后一个允许指定改变是否被保存。这里第一个参数
	我们应指定SPI_SETDESKWALLPAPER，指明我们是要设置壁纸。接下来是
	文件路径。在Vista之前必须是一个.bmp的文件。Vista和更高级的系统
	支持.jpg格式
	*************************************************/
	//SPI_SETDESKWALLPAPER参数使得壁纸改变保存并持续可见。
	if (SUCCEEDED(hr))
	{
		if (!SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)(pszFile), SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	return hr;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	SetDesktopWallpaper(L"C:\\2.jpg", Stretch);
	return 0;
}