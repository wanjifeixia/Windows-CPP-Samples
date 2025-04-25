#include <stdio.h>
#include <Windows.h>
#include <ShlObj_core.h>
#include <tchar.h>
#include <iostream>
#include <atlbase.h>
#include "WindowsUserChoice.h"

using namespace std;

void RegisterFileRelation(LPCTSTR strExt, LPCTSTR strAppName, LPCTSTR strAppKey, LPCTSTR strDefaultIcon)
{
	TCHAR strTemp[_MAX_PATH];
	HKEY hKey;

	RegCreateKey(HKEY_CLASSES_ROOT, strExt, &hKey);
	RegSetValue(hKey, _T(""), REG_SZ, strAppKey, _tcslen(strAppKey) + 1);
	RegCloseKey(hKey);

	RegCreateKey(HKEY_CLASSES_ROOT, strAppKey, &hKey);
	RegCloseKey(hKey);

	_stprintf_s(strTemp, _T("%s\\DefaultIcon"), strAppKey);
	RegCreateKey(HKEY_CLASSES_ROOT, strTemp, &hKey);
	RegSetValue(hKey, _T(""), REG_SZ, strDefaultIcon, _tcslen(strDefaultIcon) + 1);
	RegCloseKey(hKey);

	_stprintf_s(strTemp, _T("%s\\Shell"), strAppKey);
	RegCreateKey(HKEY_CLASSES_ROOT, strTemp, &hKey);
	RegSetValue(hKey, _T(""), REG_SZ, _T("Open"), _tcslen(_T("Open")) + 1);
	RegCloseKey(hKey);

	_stprintf_s(strTemp, _T("%s\\Shell\\Open\\Command"), strAppKey);
	RegCreateKey(HKEY_CLASSES_ROOT, strTemp, &hKey);
	RegSetValue(hKey, _T(""), REG_SZ, strAppName, _tcslen(strAppName) + 1);
	RegCloseKey(hKey);
}

BOOL CheckFileRelation(LPCTSTR strExt, LPCTSTR strAppName, LPCTSTR strAppKey)
{
	int nRet = FALSE;
	HKEY hExtKey;
	TCHAR strTemp[_MAX_PATH];
	DWORD dwSize = sizeof(strTemp);
	if (RegOpenKey(HKEY_CLASSES_ROOT, strExt, &hExtKey) == ERROR_SUCCESS)
	{
		RegQueryValueEx(hExtKey, NULL, NULL, NULL, (LPBYTE)strTemp, &dwSize);
		RegCloseKey(hExtKey);
		if (_tcscmp(strTemp, strAppKey) == 0)
		{
			_stprintf_s(strTemp, _T("%s\\Shell\\Open\\Command"), strAppKey);
			if (RegOpenKey(HKEY_CLASSES_ROOT, strTemp, &hExtKey) == ERROR_SUCCESS)
			{
				dwSize = sizeof(strTemp);
				RegQueryValueEx(hExtKey, NULL, NULL, NULL, (LPBYTE)strTemp, &dwSize);
				if (_tcscmp(strTemp, strAppName) == 0)
				{
					nRet = TRUE;
				}
				RegCloseKey(hExtKey);
			}
		}
	}
	return nRet;
}

//设置程序关联.REC文件
void SetFileRelation()
{
	TCHAR appPath[_MAX_PATH];
	TCHAR appName[_MAX_PATH];

	LPCTSTR NAME_APP = _T("WellTest.REC.1.0");
	LPCTSTR NAME_PROJECTFILE_EXT = _T(".REC");

	if (GetModuleFileName(NULL, appPath, _MAX_PATH) != 0)
	{
		_stprintf_s(appName, _T("\"%s\" %%1"), appPath);
		//检查注册表是否已经添加了，没有则添加
		if (!CheckFileRelation(NAME_PROJECTFILE_EXT, appName, NAME_APP))
		{
			TCHAR icoPath[_MAX_PATH];
			_stprintf_s(icoPath, _T("%s,0"), appPath);
			RegisterFileRelation(NAME_PROJECTFILE_EXT, appName, NAME_APP, icoPath);
		}
	}
}

std::wstring GetDefaultAppProgId(const wchar_t* aExt)
{
	wstring keyName;
	if (!_wcsicmp(aExt, L"http") || !_wcsicmp(aExt, L"https"))
		keyName = L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\" + wstring(aExt) + L"\\UserChoice";
	else
		keyName = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\" + wstring(aExt) + L"\\UserChoice";
 
	// 打开注册表项
	HKEY hKey;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, keyName.c_str(), 0, KEY_READ, &hKey);
	if (lRes != ERROR_SUCCESS) {
		std::wcerr << L"Failed to open registry key. Error code: " << lRes << std::endl;
		return L"";
	}

	// 查询 ProgId 值
	WCHAR szProgId[256];
	DWORD dwSize = sizeof(szProgId);
	lRes = RegQueryValueEx(hKey, L"ProgId", nullptr, nullptr, (LPBYTE)szProgId, &dwSize);
	RegCloseKey(hKey);

	if (lRes != ERROR_SUCCESS) {
		std::wcerr << L"Failed to query ProgId. Error code: " << lRes << std::endl;
		return L"";
	}

	return std::wstring(szProgId);
}

//方案一
bool SetUserChoice(const wchar_t* aExt, const wchar_t* aProgID)
{
	//设置http浏览器
	wstring keyName = L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\" + wstring(aExt) + L"\\UserChoice";
	CRegKey key;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, keyName.c_str(), KEY_ALL_ACCESS))
	{
		// generate UserChoice Hash
		static std::unique_ptr<wchar_t[]> sid = UserChoice::GetCurrentUserStringSid();
		SYSTEMTIME systemTime = {};
		GetSystemTime(&systemTime);

		auto hash = UserChoice::GenerateUserChoiceHash(aExt, sid.get(), aProgID, systemTime);
		if (hash &&
			ERROR_SUCCESS == RegDeleteKey(HKEY_CURRENT_USER, keyName.c_str()) &&
			ERROR_SUCCESS == key.Create(HKEY_CURRENT_USER, keyName.c_str()))
		{
			key.SetStringValue(L"ProgId", aProgID);
			key.SetStringValue(L"Hash", hash.get());
		}
	}
	return true;
}

static bool SetUserChoiceRegistry(const wchar_t* aExt, const wchar_t* aProgID, const bool aRegRename, const wchar_t* aHash)
{
	auto assocKeyPath = UserChoice::GetAssociationKeyPath(aExt);
	if (!assocKeyPath) {
		return false;
	}

	LSTATUS ls;
	HKEY rawAssocKey;
	ls = ::RegOpenKeyExW(HKEY_CURRENT_USER, assocKeyPath.get(), 0,
		KEY_READ | KEY_WRITE, &rawAssocKey);
	if (ls != ERROR_SUCCESS) {
		return false;
	}

	if (aRegRename) {
		// Registry keys in the HKCU should be writable by applications run by the
		// hive owning user and should not be lockable -
		// https://web.archive.org/web/20230308044345/https://devblogs.microsoft.com/oldnewthing/20090326-00/?p=18703.
		// Unfortunately some kernel drivers lock a set of protocol and file
		// association keys. Renaming a non-locked ancestor is sufficient to fix
		// this.
		ls = ::RegRenameKey(rawAssocKey, nullptr, L"EndeskTempKey");
		if (ls != ERROR_SUCCESS) {
			return false;
		}
	}

	auto subkeysUpdated = [&] {
		// Windows file association keys are read-only (Deny Set Value) for the
		// User, meaning they can not be modified but can be deleted and recreated.
		// We don't set any similar special permissions. Note: this only applies to
		// file associations, not URL protocols.
		if (aExt && aExt[0] == '.') {
			ls = ::RegDeleteKeyW(rawAssocKey, L"UserChoice");
			if (ls != ERROR_SUCCESS) {
				return false;
			}
		}

		HKEY rawUserChoiceKey;
		ls = ::RegCreateKeyExW(rawAssocKey, L"UserChoice", 0, nullptr,
			0 /* options */, KEY_READ | KEY_WRITE,
			0 /* security attributes */, &rawUserChoiceKey,
			nullptr);
		if (ls != ERROR_SUCCESS) {
			return false;
		}

		DWORD progIdByteCount = (::lstrlenW(aProgID) + 1) * sizeof(wchar_t);
		ls = ::RegSetValueExW(rawUserChoiceKey, L"ProgID", 0, REG_SZ,
			reinterpret_cast<const unsigned char*>(aProgID),
			progIdByteCount);
		if (ls != ERROR_SUCCESS) {
			return false;
		}

		DWORD hashByteCount = (::lstrlenW(aHash) + 1) * sizeof(wchar_t);
		ls = ::RegSetValueExW(rawUserChoiceKey, L"Hash", 0, REG_SZ, reinterpret_cast<const unsigned char*>(aHash), hashByteCount);
		if (ls != ERROR_SUCCESS) {
			return false;
		}

		return true;
	}();

	if (aRegRename) {
		// Rename back regardless of whether we successfully modified the subkeys to
		// minimally attempt to return the registry the way we found it. If this
		// fails the afformetioned kernel drivers have likely changed and there's
		// little we can do to anticipate what proper recovery should look like.
		ls = ::RegRenameKey(rawAssocKey, nullptr, aExt);
		if (ls != ERROR_SUCCESS) {
			return false;
		}
	}

	return subkeysUpdated;
}

int main(int argc, char* argv[])
{
	// 设置全局本地化，支持中文
	std::wcout.imbue(std::locale(""));

	if (0)
	{
		//设置程序关联.REC文件
		SetFileRelation();
		//通知文件关联已经更新
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
	}

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = 设置.pdf默认打开方式为MSEdgePDF" << std::endl;
		std::wcout << L"输入2 = 设置.pdf默认打开方式为Applications\\NOTEPAD.EXE" << std::endl;
		std::wcout << L"输入3 = 设置默认浏览器为ChromeHTML" << std::endl;
		std::wcout << L"输入4 = 设置默认浏览器为FirefoxURL-308046B0AF4A39CB" << std::endl;
		std::wcout << L"输入5 = 打开Windows的默认应用设置界面并设置Chrome为默认浏览器" << std::endl;
		std::wcout << L"输入6 = 打开Windows的默认应用设置界面并设置Firefox为默认浏览器" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			static std::unique_ptr<wchar_t[]> sid = UserChoice::GetCurrentUserStringSid();
			SYSTEMTIME systemTime = {};
			GetSystemTime(&systemTime);
			auto hash = UserChoice::GenerateUserChoiceHash(L".pdf", sid.get(), L"MSEdgePDF", systemTime);
			SetUserChoiceRegistry(L".pdf", L"MSEdgePDF", false, hash.get());

			if (GetDefaultAppProgId(L".pdf") == L"MSEdgePDF")
				std::wcout << L"设置成功" << std::endl;
			else
				std::wcout << L"设置失败" << std::endl;
		}
		else if (iInput == 2)
		{
			static std::unique_ptr<wchar_t[]> sid = UserChoice::GetCurrentUserStringSid();
			SYSTEMTIME systemTime = {};
			GetSystemTime(&systemTime);
			auto hash = UserChoice::GenerateUserChoiceHash(L".pdf", sid.get(), L"Applications\\NOTEPAD.EXE", systemTime);
			SetUserChoiceRegistry(L".pdf", L"Applications\\NOTEPAD.EXE", false, hash.get());

			if (GetDefaultAppProgId(L".pdf") == L"Applications\\NOTEPAD.EXE")
				std::wcout << L"设置成功" << std::endl;
			else
				std::wcout << L"设置失败" << std::endl;
		}
		else if (iInput == 3)
		{
			wstring strProgId = L"ChromeHTML";
			wstring strHttp = GetDefaultAppProgId(L"http");
			wstring strHttps = GetDefaultAppProgId(L"https");
			wcout << L"设置默认浏览器之前 http:" << strHttp << L"\thttps:" << strHttps << endl;

			SetUserChoice(L"http", strProgId.c_str());
			SetUserChoice(L"https", strProgId.c_str());

			strHttp = GetDefaultAppProgId(L"http");
			strHttps = GetDefaultAppProgId(L"https");
			wcout << L"设置默认浏览器之后 http:" << strHttp << L"\thttps:" << strHttps << endl;

			if (strHttp == strHttps && strHttps == strProgId)
				wcout << L"设置默认浏览器成功" << endl;
			else
				wcout << L"设置默认浏览器失败 http:" << strHttp << L" https:" << strHttps << endl;
		}
		else if (iInput == 4)
		{
			wstring strProgId = L"FirefoxURL-308046B0AF4A39CB";
			wstring strHttp = GetDefaultAppProgId(L"http");
			wstring strHttps = GetDefaultAppProgId(L"https");
			wcout << L"设置默认浏览器之前 http:" << strHttp << L"\thttps:" << strHttps << endl;

			//重命名注册表键(FireFox)
			static std::unique_ptr<wchar_t[]> sid = UserChoice::GetCurrentUserStringSid();
			SYSTEMTIME systemTime = {};
			GetSystemTime(&systemTime);
			auto hash = UserChoice::GenerateUserChoiceHash(L"http", sid.get(), strProgId.c_str(), systemTime);
			SetUserChoiceRegistry(L"http", strProgId.c_str(), true, hash.get());
			auto hash1 = UserChoice::GenerateUserChoiceHash(L"https", sid.get(), strProgId.c_str(), systemTime);
			SetUserChoiceRegistry(L"https", strProgId.c_str(), true, hash1.get());

			strHttp = GetDefaultAppProgId(L"http");
			strHttps = GetDefaultAppProgId(L"https");
			wcout << L"设置默认浏览器之后 http:" << strHttp << L"\thttps:" << strHttps << endl;

			if (strHttp == strHttps && strHttps == strProgId)
				wcout << L"设置默认浏览器成功" << endl;
			else
				wcout << L"设置默认浏览器失败 http:" << strHttp << L" https:" << strHttps << endl;
		}
		else if (iInput == 5)
		{
			//打开Windows的默认应用设置界面
			//ShellExecute(nullptr, L"open", L"ms-settings:defaultapps", nullptr, nullptr, SW_SHOWNORMAL);
			ShellExecute(nullptr, L"open", L"ms-settings:defaultapps?registeredAppMachine=Google Chrome", nullptr, nullptr, SW_SHOWNORMAL);
		}
		else if (iInput == 6)
		{
			//ShellExecute(nullptr, L"open", L"ms-settings:defaultapps?registeredAppUser=Firefox-308046B0AF4A39CB", nullptr, nullptr, SW_SHOWNORMAL);
			ShellExecute(nullptr, L"open", L"ms-settings:defaultapps?registeredAppMachine=Firefox-308046B0AF4A39CB", nullptr, nullptr, SW_SHOWNORMAL);
		}
	}
}