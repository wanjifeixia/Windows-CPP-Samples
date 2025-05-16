#include <vector>
#include <memory>
#include <type_traits>
#include <Windows.h>
#include <sddl.h>
#include <Userenv.h>
#include <iostream>
#include <Netfw.h>
#include <algorithm>

#pragma comment(lib,"Userenv")
#pragma comment(lib,"Shlwapi")
#pragma comment(lib,"kernel32")
#pragma comment(lib,"user32")
#pragma comment(lib,"Advapi32")
#pragma comment(lib,"Ole32")
#pragma comment(lib,"Shell32")

using namespace std;

typedef std::shared_ptr<std::remove_pointer<PSID>::type> SHARED_SID;

bool SetCapability(const WELL_KNOWN_SID_TYPE type, std::vector<SID_AND_ATTRIBUTES>& list, std::vector<SHARED_SID>& sidList) {
	SHARED_SID capabilitySid(new unsigned char[SECURITY_MAX_SID_SIZE]);
	DWORD sidListSize = SECURITY_MAX_SID_SIZE;
	if (::CreateWellKnownSid(type, NULL, capabilitySid.get(), &sidListSize) == FALSE) {
		return false;
	}
	if (::IsWellKnownSid(capabilitySid.get(), type) == FALSE) {
		return false;
	}
	SID_AND_ATTRIBUTES attr;
	attr.Sid = capabilitySid.get();
	attr.Attributes = SE_GROUP_ENABLED;
	list.push_back(attr);
	sidList.push_back(capabilitySid);
	return true;
}

static bool MakeWellKnownSIDAttributes(std::vector<SID_AND_ATTRIBUTES>& capabilities, std::vector<SHARED_SID>& capabilitiesSidList)
{

	const WELL_KNOWN_SID_TYPE capabilitiyTypeList[] = {
		WinCapabilityInternetClientSid, WinCapabilityInternetClientServerSid, WinCapabilityPrivateNetworkClientServerSid,
		WinCapabilityPicturesLibrarySid, WinCapabilityVideosLibrarySid, WinCapabilityMusicLibrarySid,
		WinCapabilityDocumentsLibrarySid, WinCapabilitySharedUserCertificatesSid, WinCapabilityEnterpriseAuthenticationSid,
		WinCapabilityRemovableStorageSid,
	};
	for (auto type : capabilitiyTypeList) {
		if (!SetCapability(type, capabilities, capabilitiesSidList)) {
			return false;
		}
	}
	return true;
}


HRESULT AppContainerLauncherProcess(LPCWSTR app, LPCWSTR cmdArgs, LPCWSTR workDir)
{
	wchar_t appContainerName[] = L"Phoenix.Container.AppContainer.Profile.v1.test";
	wchar_t appContainerDisplayName[] = L"Phoenix.Container.AppContainer.Profile.v1.test\0";
	wchar_t appContainerDesc[] = L"Phoenix Container Default AppContainer Profile  Test,Revision 1\0";
	DeleteAppContainerProfile(appContainerName);///Remove this AppContainerProfile
	std::vector<SID_AND_ATTRIBUTES> capabilities;
	std::vector<SHARED_SID> capabilitiesSidList;
	if (!MakeWellKnownSIDAttributes(capabilities, capabilitiesSidList))
		return S_FALSE;
	PSID sidImpl;
	HRESULT hr = ::CreateAppContainerProfile(appContainerName,
		appContainerDisplayName,
		appContainerDesc,
		(capabilities.empty() ? NULL : &capabilities.front()), capabilities.size(), &sidImpl);
	if (hr != S_OK) {
		std::cout << "CreateAppContainerProfile Failed" << std::endl;
		return hr;
	}
	wchar_t* psArgs = nullptr;
	psArgs = _wcsdup(cmdArgs);
	PROCESS_INFORMATION pi;
	STARTUPINFOEX siex = { sizeof(STARTUPINFOEX) };
	siex.StartupInfo.cb = sizeof(STARTUPINFOEXW);
	SIZE_T cbAttributeListSize = 0;
	BOOL bReturn = InitializeProcThreadAttributeList(
		NULL, 3, 0, &cbAttributeListSize);
	siex.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, cbAttributeListSize);
	bReturn = InitializeProcThreadAttributeList(siex.lpAttributeList, 3, 0, &cbAttributeListSize);
	SECURITY_CAPABILITIES sc;
	sc.AppContainerSid = sidImpl;
	sc.Capabilities = (capabilities.empty() ? NULL : &capabilities.front());
	sc.CapabilityCount = capabilities.size();
	sc.Reserved = 0;
	if (UpdateProcThreadAttribute(siex.lpAttributeList, 0,
		PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES,
		&sc,
		sizeof(sc),
		NULL, NULL) == FALSE)
	{
		goto Cleanup;
	}
	BOOL bRet;
	bRet = CreateProcessW(app, psArgs, nullptr, nullptr,
		FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, workDir, reinterpret_cast<LPSTARTUPINFOW>(&siex), &pi);
	::CloseHandle(pi.hThread);
	::CloseHandle(pi.hProcess);
Cleanup:
	DeleteProcThreadAttributeList(siex.lpAttributeList);
	DeleteAppContainerProfile(appContainerName);
	free(psArgs);
	FreeSid(sidImpl);
	return hr;
}

typedef DWORD(*pNetworkIsolationEnumAppContainers)(
	_In_  DWORD                        Flags,
	_Out_ DWORD* pdwNumPublicAppCs,
	_Out_ PINET_FIREWALL_APP_CONTAINER* ppPublicAppCs
	);
typedef DWORD(*pNetworkIsolationFreeAppContainers)(
	_In_ PINET_FIREWALL_APP_CONTAINER pPublicAppCs
	);

void LaunchSpecifcApp(wstring* pfn)
{
	TCHAR szCommandLine[1024];
	wsprintf(szCommandLine, L"explorer.exe shell:AppsFolder\\%ws!App", (*pfn).c_str());
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = TRUE;

	BOOL bRet = ::CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
}

void LaunchUWPApp(const LPCWSTR strAppContainerName)
{
	vector<wstring> uwpApps;
	uwpApps.push_back(strAppContainerName);

	HMODULE FirewallAPIModule;
	FirewallAPIModule = (LoadLibrary(L"FirewallAPI.dll"));

	auto EnumAppContainersProc = pNetworkIsolationEnumAppContainers(GetProcAddress(FirewallAPIModule, "NetworkIsolationEnumAppContainers"));
	auto FreeAppContainersProc = pNetworkIsolationFreeAppContainers(GetProcAddress(FirewallAPIModule, "NetworkIsolationFreeAppContainers"));

	DWORD pdwNumPublicAppCs = 0;
	PINET_FIREWALL_APP_CONTAINER ppPublicAppCs = NULL;
	HRESULT re = EnumAppContainersProc(0, &pdwNumPublicAppCs, &ppPublicAppCs);

	for (int i = 0; i < pdwNumPublicAppCs; i++)
	{
		auto appContainer = ppPublicAppCs[i];
		for (int j = 0; j < uwpApps.size(); j++)
		{
			if (uwpApps.at(j) == appContainer.appContainerName)
			{
				//launch it;
				auto temp = uwpApps.at(j);
				LaunchSpecifcApp(&temp);
			}
		}
	}
	FreeAppContainersProc(ppPublicAppCs);
	FreeLibrary(FirewallAPIModule);
	vector<wstring>().swap(uwpApps);
}

vector<wstring> EnumAppContainers()
{
	vector<wstring> uwpApps;

	// 1. 安全加载DLL
	HMODULE hFirewallAPI = LoadLibraryExW(L"FirewallAPI.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hFirewallAPI) {
		// 可添加GetLastError()日志
		return uwpApps;
	}

	// 2. 使用类型安全的函数指针定义
	using EnumAppContainersFn = decltype(NetworkIsolationEnumAppContainers)*;
	using FreeAppContainersFn = decltype(NetworkIsolationFreeAppContainers)*;

	// 3. 安全获取函数指针
	auto EnumAppContainersProc = reinterpret_cast<EnumAppContainersFn>(
		GetProcAddress(hFirewallAPI, "NetworkIsolationEnumAppContainers"));
	auto FreeAppContainersProc = reinterpret_cast<FreeAppContainersFn>(
		GetProcAddress(hFirewallAPI, "NetworkIsolationFreeAppContainers"));

	if (!EnumAppContainersProc || !FreeAppContainersProc) {
		FreeLibrary(hFirewallAPI);
		return uwpApps;
	}

	// 4. 枚举应用容器
	DWORD dwCount = 0;
	PINET_FIREWALL_APP_CONTAINER pAppContainers = nullptr;
	const HRESULT hr = EnumAppContainersProc(0, &dwCount, &pAppContainers);

	if (SUCCEEDED(hr) && dwCount > 0 && pAppContainers) {
		uwpApps.reserve(dwCount);  // 预分配空间提升性能

		// 5. 使用正确类型的循环变量
		for (DWORD i = 0; i < dwCount; ++i) {
			const auto& container = pAppContainers[i];

			// 6. 安全处理可能为空的字符串
			if (container.appContainerName) {
				uwpApps.emplace_back(container.appContainerName);
			}
		}
	}

	// 7. 安全释放资源
	if (pAppContainers) {
		FreeAppContainersProc(pAppContainers);
	}
	FreeLibrary(hFirewallAPI);

	return uwpApps;
}

int main(int argc, wchar_t* argv[])
{
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));
	std::wcout.imbue(std::locale(""));

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = 创建一个应用程序容器（AppContainer）沙箱环境运行Win32程序:TestDoDragDrop.exe" << std::endl;
		std::wcout << L"输入2 = 启动UWP程序 - 记事本" << std::endl;
		std::wcout << L"输入3 = 枚举本机所有的UWP程序" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			std::wstring strPath = L"C:\\Users\\46043\\Desktop\\TestDoDragDrop.exe";
			std::wcout << L"Start AppContainer App: " << strPath.c_str() << L"\t Return Code[HRESULT]: "
				<< AppContainerLauncherProcess(nullptr, strPath.c_str(), nullptr) << std::endl;
		}
		else if (iInput == 2)
		{
			// 启动记事本
			LaunchUWPApp(L"Microsoft.WindowsNotepad_8wekyb3d8bbwe");
		}
		else if (iInput == 3)
		{
			// 枚举UWP程序
			vector<wstring> vtUWPApp = EnumAppContainers();
			for (auto& strApp : vtUWPApp)
			{
				std::wcout << L"AppContainer App: " << strApp.c_str() << std::endl;
			}
		}
	}
	return 0;
}