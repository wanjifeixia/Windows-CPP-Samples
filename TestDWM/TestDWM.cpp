#include <iostream>
#include <windows.h>

typedef HRESULT(WINAPI* P_DwmIsCompositionEnabled)(BOOL* enabled);
typedef HRESULT(WINAPI* P_DwmGetWindowAttribute)(HWND hWnd, DWORD dwAttribute, void* pvAttribute, DWORD cbAttribute);

P_DwmIsCompositionEnabled pDwmIsCompositionEnabled = NULL;
P_DwmGetWindowAttribute pDwmGetWindowAttribute = NULL;

int main()
{
	HMODULE dwmapi = LoadLibrary(L"dwmapi.dll");
	if (dwmapi)
	{
		pDwmIsCompositionEnabled = (P_DwmIsCompositionEnabled)GetProcAddress(dwmapi, "DwmIsCompositionEnabled");
		pDwmGetWindowAttribute = (P_DwmGetWindowAttribute)GetProcAddress(dwmapi, "DwmGetWindowAttribute");
	}

	while (true)
	{
		BOOL dwmEnabled = FALSE;
		HRESULT hr = pDwmIsCompositionEnabled(&dwmEnabled);
		if (SUCCEEDED(hr) && dwmEnabled)
		{
			WCHAR szPrint[128] = { 0 };
			RECT rect;
			HWND hWnd = FindWindow(NULL, L"钉钉");

			const ULONG DWMWA_EXTENDED_FRAME_BOUNDS = 9;
			hr = pDwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(RECT));
			wsprintf(szPrint, L"DwmGetWindowAttribute HWND:0x%08X rect.left:%d, rect.top:%d, rect.right:%d, rect.bottom:%d\n", hWnd, rect.left, rect.top, rect.right, rect.bottom);
			wprintf(szPrint);

			GetWindowRect(hWnd, &rect);
			wsprintf(szPrint, L"GetWindowRect HWND:0x%08X rect.left:%d, rect.top:%d, rect.right:%d, rect.bottom:%d\n\n", hWnd, rect.left, rect.top, rect.right, rect.bottom);
			wprintf(szPrint);

			Sleep(1000);
		}
	}
}