// demote, aka "scratch"
// from http://blogs.msdn.com/b/oldnewthing/archive/2013/11/18/10468726.aspx

#include <shlobj.h>
#include <atlbase.h>
#include <iostream>

void FindDesktopFolderView(REFIID riid, void** ppv)
{
	CComPtr<IShellWindows> spShellWindows;
	spShellWindows.CoCreateInstance(CLSID_ShellWindows);

	CComVariant vtLoc(CSIDL_DESKTOP);
	CComVariant vtEmpty;
	long lhwnd;
	CComPtr<IDispatch> spdisp;
	spShellWindows->FindWindowSW(
		&vtLoc, &vtEmpty,
		SWC_DESKTOP, &lhwnd, SWFO_NEEDDISPATCH, &spdisp);

	CComPtr<IShellBrowser> spBrowser;
	CComQIPtr<IServiceProvider>(spdisp)->
		QueryService(SID_STopLevelBrowser,
			IID_PPV_ARGS(&spBrowser));

	CComPtr<IShellView> spView;
	spBrowser->QueryActiveShellView(&spView);

	spView->QueryInterface(riid, ppv);
}

void GetDesktopAutomationObject(REFIID riid, void** ppv)
{
	CComPtr<IShellView> spsv;
	FindDesktopFolderView(IID_PPV_ARGS(&spsv));
	CComPtr<IDispatch> spdispView;
	spsv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&spdispView));
	spdispView->QueryInterface(riid, ppv);
}

void ShellExecuteFromExplorer(
	PCWSTR pszFile,
	PCWSTR pszParameters = nullptr,
	PCWSTR pszDirectory = nullptr,
	PCWSTR pszOperation = nullptr,
	int nShowCmd = SW_SHOWNORMAL)
{
	CComPtr<IShellFolderViewDual> spFolderView;
	GetDesktopAutomationObject(IID_PPV_ARGS(&spFolderView));
	CComPtr<IDispatch> spdispShell;
	spFolderView->get_Application(&spdispShell);

	CComQIPtr<IShellDispatch2>(spdispShell)
		->ShellExecute(CComBSTR(pszFile),
			CComVariant(pszParameters ? pszParameters : L""),
			CComVariant(pszDirectory ? pszDirectory : L""),
			CComVariant(pszOperation ? pszOperation : L""),
			CComVariant(nShowCmd));
}

class CCoInitialize {
public:
	CCoInitialize() : m_hr(CoInitialize(NULL)) {}
	~CCoInitialize() { if (SUCCEEDED(m_hr)) CoUninitialize(); }
	operator HRESULT() const { return m_hr; }
	HRESULT m_hr;
};

#include <windows.h>
#include <stdio.h>
#include <iostream>

BOOL setPrivilege(LPCTSTR priv) {
	HANDLE token;
	TOKEN_PRIVILEGES tp;
	LUID luid = { 0 };
	BOOL res = TRUE;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!LookupPrivilegeValue(NULL, priv, &luid)) res = FALSE;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) res = FALSE;
	if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) res = FALSE;
	return res;
}

HANDLE getToken(DWORD pid) {
	HANDLE cToken = NULL;
	HANDLE ph = NULL;
	if (pid == 0) {
		ph = GetCurrentProcess();
	}
	else {
		ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, true, pid);
	}
	if (!ph) cToken = (HANDLE)NULL;
	BOOL res = OpenProcessToken(ph, MAXIMUM_ALLOWED, &cToken);
	if (!res) cToken = (HANDLE)NULL;
	return cToken;
}

BOOL createProcess(HANDLE token, LPCWSTR app) {
	HANDLE dToken = NULL;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	BOOL res = TRUE;
	ZeroMemory(&si, sizeof(STARTUPINFOW));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFOW);

	res = DuplicateTokenEx(token, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &dToken);
	res = CreateProcessWithTokenW(dToken, 0, app, NULL, 0, NULL, NULL, &si, &pi);
	return res;
}

int main(int argc, _TCHAR* argv[])
{
	// 设置本地化
	std::locale::global(std::locale(""));

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = 通过Explorer降权启动Notepad" << std::endl;
		std::wcout << L"输入2 = 调用IShellWindows::FindWindowSW" << std::endl;
		std::wcout << L"输入3 = 调用TestCreateProcessWithToken降权启动cmd" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			CCoInitialize init;
			ShellExecuteFromExplorer(L"notepad.exe", L"qqqqq");
		}
		else if (iInput == 2)
		{
			CCoInitialize init;
			CComPtr<IShellWindows> spShellWindows;
			MessageBox(0, L"开始CoCreateInstance CLSID_ShellWindows", 0, 0);
			HRESULT hr = spShellWindows.CoCreateInstance(CLSID_ShellWindows);

			CComVariant vtLoc(CSIDL_DESKTOP);
			CComVariant vtEmpty;
			long lhwnd;
			CComPtr<IDispatch> spdisp;
			MessageBox(0, L"开始查找桌面句柄", 0, 0);
			spShellWindows->FindWindowSW(
				&vtLoc, &vtEmpty,
				SWC_DESKTOP, &lhwnd, SWFO_NEEDDISPATCH, &spdisp);

			if (lhwnd)
			{
				MessageBox(0, L"找到桌面句柄", 0, 0);
			}
			else
			{
				MessageBox(0, L"Error 未找到桌面句柄", 0, 0);
			}
		}
		else if (iInput == 3)
		{
			if (!setPrivilege(SE_DEBUG_NAME)) return -1;
			HWND hWnd = GetShellWindow();
			std::wcout << L"GetShellWindow:" << hWnd << std::endl;
			DWORD pid = 0;
			GetWindowThreadProcessId(hWnd, &pid);
			std::wcout << L"GetWindowThreadProcessId:" << pid << std::endl;
			HANDLE cToken = getToken(pid);
			createProcess(cToken, L"C:\\Windows\\system32\\cmd.exe");
		}
	}
	return 0;
}