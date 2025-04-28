// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <oleidl.h>
#include <Shlobj.h>

void InitializeDLL(HMODULE hModule);
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        InitializeDLL(hModule);
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

HHOOK wndProcHook = NULL;
HINSTANCE hInst;
BOOL gIsTargetProcess = TRUE;
BOOL gIsTargetProcessChecked = FALSE;
#define TARGET_PROCESS_NAME                 _T("explorer.exe")
#define TARGET_WINDOW_CLASS_GRANTPA         _T("TXGuiFoundation")
#define TARGET_WINDOW_CLASS                 _T("DirectUIHWND")
#define TARGET_WINDOW_CLASS_PARENT          _T("SHELLDLL_DefView")
#define TARGET_WINDOW_TEXT                  _T("QQ")
BOOL CheckTargetProcess(TCHAR* szTargetProcessName)
{
    TCHAR path[MAX_PATH] = _T("");
    HMODULE hMod = GetModuleHandle(NULL);
    GetModuleFileName(hMod, path, _countof(path));

    TCHAR* name = &path[_tcslen(path)];
    while (name > path && *name != _T('\\')) {
        *name = _totlower(*name);
        name--;
    }
    name++;
    if (_tccmp(name, szTargetProcessName) == 0) {
        return TRUE;

    }
    //EnumWindows(EnumWindowsProc_CheckTargetProcess, NULL);
    return FALSE;
}

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

IDataObject* g_pDataObject = NULL;
void HookDropTarget(HWND hwnd)
{
    POINTL  point = { 1,1 };
    DWORD   dwEffect = DROPEFFECT_COPY;// | DROPEFFECT_MOVE;
    CHAR    szShow[0x80] = { 0 };
    IDropTarget* target = (IDropTarget*)GetProp(hwnd, _T("OleDropTargetInterface"));
    if (target)
    {
        if (!g_pDataObject)
            GetUIObjectOfFile(NULL, L"C:\\Users\\yixian\\Desktop\\W11ClassicMenu.zip", IID_IDataObject, (void**)&g_pDataObject);

        if (g_pDataObject)
        {
            HRESULT hResult = target->DragEnter(g_pDataObject, MK_LBUTTON, point, &dwEffect);
            if (SUCCEEDED(hResult) && (dwEffect & DROPEFFECT_COPY))
            {
                point.x++;
                point.y++;
                hResult = target->DragOver(MK_LBUTTON, point, &dwEffect);
                if (SUCCEEDED(hResult) && (dwEffect & DROPEFFECT_COPY))
                {
                    hResult = target->Drop(g_pDataObject, 0, point, &dwEffect);
                }
            }
        }
    }
}
LRESULT CALLBACK MsgHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (!gIsTargetProcess) {
        return CallNextHookEx(wndProcHook, nCode, wParam, lParam);
    }
    if (!gIsTargetProcessChecked) {
        gIsTargetProcessChecked = TRUE;
        gIsTargetProcess = CheckTargetProcess((TCHAR*)TARGET_PROCESS_NAME);
    }

    if (nCode == HC_ACTION) {
        LPCWPRETSTRUCT p = (LPCWPRETSTRUCT)lParam;

        /*if (GetProp(p->hwnd,_T("OleDropTargetInterface")) != NULL) {
            DWORD   dwStyle = GetWindowLong(p->hwnd,GWL_STYLE);
            //if(dwStyle & WS_VISIBLE) {
                if(GetProp(p->hwnd, _T("AlterDnD Checked")) == NULL) {
                    TCHARclassName[256] = { _T('\0') };
                    GetClassName(p->hwnd,className, sizeof(className));
                    if(_tcscmp(className, TARGET_WINDOW_CLASS) == 0) {
                        GetWindowText(p->hwnd,className, 0x100);
                        staticint static_i_b = 0;
                        if(_tcscmp(className, TARGET_WINDOW_TEXT) != 0 && !static_i_b) {
                            //ShowInterger((DWORD64)p->hwnd);
                            //static_i_b= 1;
                            HookDropTarget(p->hwnd);
                        }
                    }
                    SetProp(p->hwnd,_T("AlterDnD Checked"), p->hwnd);
                }
            //}
        }*/
        if (GetProp(p->hwnd, _T("OleDropTargetInterface")) != NULL) {
            DWORD   dwStyle = GetWindowLong(p->hwnd, GWL_STYLE);
            if (dwStyle & WS_VISIBLE) {
                if (GetProp(p->hwnd, _T("AutoDrop")) == NULL) {
                    SetProp(p->hwnd, _T("AutoDrop"), p->hwnd);
                    TCHAR className[256] = { _T('\0') };
                    GetClassName(p->hwnd, className, sizeof(className));
                    if (_tcscmp(className, TARGET_WINDOW_CLASS) == 0) {

                        HWND hParent = GetParent(p->hwnd);
                        GetClassName(hParent, className, 0x100);
                        if (_tcscmp(className, TARGET_WINDOW_CLASS_PARENT) == 0) {
                            HookDropTarget(p->hwnd);
                        }
                    }

                }
            }
        }
    }
    return CallNextHookEx(wndProcHook, nCode, wParam, lParam);
}

extern "C" _declspec(dllexport) BOOL InstallHook()
{
    wndProcHook = SetWindowsHookEx(WH_CALLWNDPROCRET, MsgHookProc, hInst, 0);
    return TRUE;
}

extern "C" _declspec(dllexport) void UninstallHook()
{
    if (wndProcHook != NULL) {
        UnhookWindowsHookEx(wndProcHook);
        wndProcHook = NULL;
    }
}

void InitializeDLL(HMODULE hModule)
{
    hInst = hModule;
}