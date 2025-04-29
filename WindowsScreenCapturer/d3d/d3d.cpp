// d3d.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <fstream>
using namespace std;

#include <d3d9helper.h>
#include <gdiplus.h>
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib, "gdiplus.lib")

wstring Format(LPCWSTR pszFormat, ...)
{
	wstring ret;
	va_list ptr;
	va_start(ptr, pszFormat);
	int nlen = _vscwprintf(pszFormat, ptr);
	if (nlen > 0)
	{
		ret.resize(nlen + 1);
		nlen = vswprintf_s((wchar_t*)ret.data(), nlen + 1, pszFormat, ptr);
		ret.resize(nlen);
	}
	va_end(ptr);
	return std::move(ret);
}

static size_t WINAPI HookVtbl(void* pObject, size_t classIdx, size_t methodIdx, size_t newMethod)
{
	if (pObject == NULL || newMethod == NULL)
		return NULL;

	size_t** vtbl = (size_t**)pObject;
	DWORD oldProtect = 0;
	size_t oldMethod = vtbl[classIdx][methodIdx];

	if (!VirtualProtect(vtbl[classIdx] + methodIdx, sizeof(size_t*), PAGE_READWRITE, &oldProtect))
	{
		return NULL;
	}

	//修改虚函数
	vtbl[classIdx][methodIdx] = newMethod;

	if (!VirtualProtect(vtbl[classIdx] + methodIdx, sizeof(size_t*), oldProtect, &oldProtect))
	{
		return NULL;
	}
	return oldMethod;
}

//水印
void DrawWatermark(int width, int height, int stride, void* pData)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	Gdiplus::Bitmap bmp(width, height, stride, PixelFormat32bppARGB, (BYTE*)pData);
	if (bmp.GetLastStatus() == Gdiplus::Ok)
	{
		Gdiplus::Graphics graphics(&bmp);
		Gdiplus::FontFamily fontFamily(L"Arial");
		Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

		Gdiplus::SolidBrush solidBrush(Gdiplus::Color(100, 255, 0, 100));
		std::wstring text(L"Hello, world!\r\n撒大声地\r\n嘎嘎嘎");

		for (int i = 50; i < width; i += 200)
		{
			for (int k = 50; k < height; k += 150)
			{
				Gdiplus::Matrix matrix;
				matrix.RotateAt(-45, Gdiplus::PointF(i, k));
				graphics.SetTransform(&matrix);

				graphics.DrawString(text.c_str(), -1, &font, Gdiplus::PointF(i, k), &solidBrush);
			}
		}
	}
}

LPDIRECT3D9 g_pObjD3d = NULL;
IDirect3DDevice9* g_pObjD3dDevice = NULL;
IDirect3DSurface9* g_pObjSurface = NULL;

typedef HRESULT(STDMETHODCALLTYPE* IDIRECT3DDEVICE9_GETFRONTBUFFERDATA)(IDirect3DDevice9* This, THIS_ UINT iSwapChain, IDirect3DSurface9* pDestSurface);

IDIRECT3DDEVICE9_GETFRONTBUFFERDATA Old_IDirect3DDevice9_GetFrontBufferData;

HRESULT STDMETHODCALLTYPE New_IDirect3DDevice9_GetFrontBufferData(IDirect3DDevice9* This, THIS_ UINT iSwapChain, IDirect3DSurface9* pDestSurface)
{
	HRESULT hr = Old_IDirect3DDevice9_GetFrontBufferData(This, iSwapChain, pDestSurface);

	D3DSURFACE_DESC desc;
	g_pObjSurface->GetDesc(&desc);

	D3DLOCKED_RECT lockedRect;
	g_pObjSurface->LockRect(&lockedRect, NULL,
		D3DLOCK_NO_DIRTY_UPDATE |
		D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY);

	//水印
	DrawWatermark(desc.Width, desc.Height, lockedRect.Pitch, lockedRect.pBits);

	g_pObjSurface->UnlockRect();
	return hr;
}

typedef IDirect3D9* (WINAPI* DIRECT3DCREATE9)(UINT SDKVersion);
int initDirect3D()
{
	DEVMODEW dm = { 0 };
	dm.dmSize = sizeof(DEVMODEW);
	EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm);
	int nScreenWidth = dm.dmPelsWidth;//GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = dm.dmPelsHeight;//GetSystemMetrics(SM_CYSCREEN);
	HWND hDesktopWnd = GetDesktopWindow();

	HMODULE module = LoadLibrary(L"d3d911.dll");
	if (!module)
		module = LoadLibrary(L"d3d9.dll");

	DIRECT3DCREATE9 Direct3DCreate9 = (DIRECT3DCREATE9)GetProcAddress(module, "Direct3DCreate9");
	g_pObjD3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (g_pObjD3d == NULL)
	{
		return -1;
	}

	//设置呈现参数
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.BackBufferWidth = nScreenWidth;
	d3dpp.BackBufferHeight = nScreenHeight;
	d3dpp.hDeviceWindow = hDesktopWnd;

	//创建D3D的设备
	g_pObjD3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hDesktopWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&g_pObjD3dDevice
	);

	if (g_pObjD3dDevice == NULL)
	{
		return -1;
	}

	//Old_IDirect3DDevice9_GetFrontBufferData = (IDIRECT3DDEVICE9_GETFRONTBUFFERDATA)HookVtbl(g_pObjD3dDevice, 0, 33, (size_t)New_IDirect3DDevice9_GetFrontBufferData);

	IDirect3DDevice9* g_pObjD3dDevice111 = NULL;
	g_pObjD3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hDesktopWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&g_pObjD3dDevice111
	);

	if (D3D_OK != g_pObjD3dDevice->CreateOffscreenPlainSurface(nScreenWidth, nScreenHeight,
		D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH,
		&g_pObjSurface, NULL))
	{
		return -1;
	}

	return 0;
}

void GetDesktopImageByDx(char* buffer)
{
	DEVMODEW dm = { 0 };
	dm.dmSize = sizeof(DEVMODEW);
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
	int nScreenWidth = dm.dmPelsWidth;//GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = dm.dmPelsHeight;//GetSystemMetrics(SM_CYSCREEN);

	g_pObjD3dDevice->GetFrontBufferData(0, g_pObjSurface);
	D3DLOCKED_RECT lockedRect;
	g_pObjSurface->LockRect(&lockedRect, NULL,
		D3DLOCK_NO_DIRTY_UPDATE |
		D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY);
	for (int i = 0; i < nScreenHeight; i++)
	{
		memcpy((BYTE*)buffer + i * nScreenWidth * 32 / 8,
			(BYTE*)lockedRect.pBits + i * lockedRect.Pitch,
			nScreenWidth * 32 / 8);
	}

	bool save_img = true;
	if (save_img) {

		char exePath[MAX_PATH] = { 0 };
		GetCurrentDirectoryA(MAX_PATH, exePath);
		std::cout << "d3d截图:" << exePath << "\\img_d3d.bmp" << std::endl;

		char file[100] = "";
		sprintf_s(file, "img_d3d.bmp");
		ofstream fout;
		fout.open(file, std::ios::binary);

		// construct bitmap
		BITMAPINFOHEADER bmif;
		bmif.biSize = sizeof(BITMAPINFOHEADER);
		bmif.biHeight = nScreenHeight;
		bmif.biWidth = nScreenWidth;
		bmif.biSizeImage = bmif.biWidth * bmif.biHeight * 4;
		bmif.biPlanes = 1;
		bmif.biBitCount = (WORD)(bmif.biSizeImage / bmif.biHeight / bmif.biWidth * 8);
		bmif.biCompression = BI_RGB;

		BITMAPFILEHEADER bmfh;
		LONG offBits = sizeof(BITMAPFILEHEADER) + bmif.biSize;
		bmfh.bfType = 0x4d42; // "BM"
		bmfh.bfOffBits = offBits;
		bmfh.bfSize = offBits + bmif.biSizeImage;
		bmfh.bfReserved1 = 0;
		bmfh.bfReserved2 = 0;

		fout.write((const char*)&bmfh, sizeof(BITMAPFILEHEADER));
		fout.write((const char*)&bmif, sizeof(BITMAPINFOHEADER));
		// construct bitmap

		fout.write((char*)lockedRect.pBits, lockedRect.Pitch * nScreenHeight);
		fout.close();
	}

	g_pObjSurface->UnlockRect();
}

void termDirect3D()
{
	if (NULL != g_pObjSurface)
	{
		g_pObjSurface->Release();
		g_pObjSurface = NULL;
	}

	if (NULL != g_pObjD3dDevice)
	{
		g_pObjD3dDevice->Release();
		g_pObjD3dDevice = NULL;
	}

	if (NULL != g_pObjD3d)
	{
		g_pObjD3d->Release();
		g_pObjD3d = NULL;
	}
}

int main()
{
	initDirect3D();
	char* buffer = (char*)malloc(1920 * 1080 * 4);
	GetDesktopImageByDx(buffer);
	termDirect3D();
}