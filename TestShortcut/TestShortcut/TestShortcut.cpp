#include <windows.h>
#include <shlobj.h>
#include <wchar.h>
#include <stdio.h>
#include <xlocale>
#include <iostream>
#include <Shlwapi.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Shlwapi.lib")

// 获取快捷方式目标路径的函数
bool GetShortcutTarget(const wchar_t* shortcutPath, wchar_t* targetPath, wchar_t* szArgu, wchar_t* szDir)
{
	IShellLinkW* pShellLink = nullptr;
	IPersistFile* pPersistFile = nullptr;

	// 初始化 COM
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		wprintf(L"COM 初始化失败: 0x%08X\n", hr);
		return false;
	}

	// 创建 IShellLink 接口实例
	hr = CoCreateInstance(
		CLSID_ShellLink,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_IShellLinkW,
		(void**)&pShellLink
	);

	if (FAILED(hr)) {
		wprintf(L"创建 ShellLink 对象失败: 0x%08X\n", hr);
		CoUninitialize();
		return false;
	}

	// 获取 IPersistFile 接口
	hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
	if (FAILED(hr)) {
		wprintf(L"获取 IPersistFile 接口失败: 0x%08X\n", hr);
		pShellLink->Release();
		CoUninitialize();
		return false;
	}

	// 加载快捷方式文件
	hr = pPersistFile->Load(shortcutPath, STGM_READ);
	if (FAILED(hr)) {
		wprintf(L"加载快捷方式文件失败: 0x%08X\n", hr);
		pPersistFile->Release();
		pShellLink->Release();
		CoUninitialize();
		return false;
	}

	// 获取目标路径
	hr = pShellLink->GetPath(targetPath, MAX_PATH, nullptr, SLGP_UNCPRIORITY);
	if (FAILED(hr)) {
		wprintf(L"获取路径失败: 0x%08X\n", hr);
		pPersistFile->Release();
		pShellLink->Release();
		CoUninitialize();
		return false;
	}

	// 获取参数
	pShellLink->GetArguments(szArgu, MAX_PATH);
	if (FAILED(hr))
	{
		wprintf(L"获取参数失败: 0x%08X\n", hr);
		pPersistFile->Release();
		pShellLink->Release();
		CoUninitialize();
		return false;
	}

	// 获取工作目录
	pShellLink->GetWorkingDirectory(szDir, MAX_PATH);
	if (FAILED(hr))
	{
		wprintf(L"获取工作目录失败: 0x%08X\n", hr);
		pPersistFile->Release();
		pShellLink->Release();
		CoUninitialize();
		return false;
	}

	// 清理资源
	pPersistFile->Release();
	pShellLink->Release();
	CoUninitialize();

	return true;
}

/**
 * @brief 创建快捷方式
 * @param targetPath   目标文件路径（如 "C:\\Program Files\\App\\app.exe"）
 * @param lnkPath      快捷方式保存路径（如 "C:\\Users\\User\\Desktop\\MyApp.lnk"）
 * @param arguments    启动参数（可选，如 "-mode=debug"）
 * @param workingDir   工作目录（可选）
 * @param description  快捷方式描述（可选）
 * @param iconPath     图标路径（可选，如 "C:\\app.ico"）
 * @param iconIndex    图标索引（默认0）
 * @return 成功返回 true，失败返回 false
 */
bool CreateShortcut(
	const wchar_t* targetPath,
	const wchar_t* lnkPath,
	const wchar_t* arguments = L"",
	const wchar_t* workingDir = nullptr,
	const wchar_t* description = L"",
	const wchar_t* iconPath = nullptr,
	int iconIndex = 0
) {
	IShellLinkW* pShellLink = nullptr;
	IPersistFile* pPersistFile = nullptr;
	HRESULT hr;

	// 初始化 COM
	hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		std::wcerr << L"COM 初始化失败: 0x" << std::hex << hr << std::endl;
		return false;
	}

	// 创建 IShellLink 对象
	hr = CoCreateInstance(
		CLSID_ShellLink,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_IShellLinkW,
		(void**)&pShellLink
	);
	if (FAILED(hr)) {
		std::wcerr << L"创建 ShellLink 失败: 0x" << std::hex << hr << std::endl;
		CoUninitialize();
		return false;
	}

	// 设置目标路径
	hr = pShellLink->SetPath(targetPath);
	if (FAILED(hr)) {
		std::wcerr << L"设置目标路径失败: 0x" << std::hex << hr << std::endl;
		goto cleanup;
	}

	// 设置启动参数
	if (arguments && wcslen(arguments) > 0) {
		hr = pShellLink->SetArguments(arguments);
		if (FAILED(hr)) {
			std::wcerr << L"设置参数失败: 0x" << std::hex << hr << std::endl;
			goto cleanup;
		}
	}

	// 设置工作目录
	if (workingDir && wcslen(workingDir) > 0) {
		hr = pShellLink->SetWorkingDirectory(workingDir);
		if (FAILED(hr)) {
			std::wcerr << L"设置工作目录失败: 0x" << std::hex << hr << std::endl;
			goto cleanup;
		}
	}

	// 设置描述
	if (description && wcslen(description) > 0) {
		hr = pShellLink->SetDescription(description);
		if (FAILED(hr)) {
			std::wcerr << L"设置描述失败: 0x" << std::hex << hr << std::endl;
			goto cleanup;
		}
	}

	// 设置图标
	if (iconPath && wcslen(iconPath) > 0) {
		hr = pShellLink->SetIconLocation(iconPath, iconIndex);
		if (FAILED(hr)) {
			std::wcerr << L"设置图标失败: 0x" << std::hex << hr << std::endl;
			goto cleanup;
		}
	}

	// 保存快捷方式文件
	hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
	if (FAILED(hr)) {
		std::wcerr << L"获取 IPersistFile 失败: 0x" << std::hex << hr << std::endl;
		goto cleanup;
	}

	hr = pPersistFile->Save(lnkPath, TRUE);
	if (FAILED(hr)) {
		std::wcerr << L"保存快捷方式失败: 0x" << std::hex << hr << std::endl;
		goto cleanup;
	}

cleanup:
	if (pPersistFile) pPersistFile->Release();
	if (pShellLink) pShellLink->Release();
	CoUninitialize();
	return SUCCEEDED(hr);
}

bool ModifyShortcutTarget(const wchar_t* lnkPath, const wchar_t* newTargetPath)
{
	IShellLinkW* pShellLink = nullptr;
	IPersistFile* pPersistFile = nullptr;
	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr)) return false;

	// 1. 创建 IShellLink 对象
	hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&pShellLink);
	if (FAILED(hr)) {
		CoUninitialize();
		return false;
	}

	// 2. 加载现有快捷方式
	hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
	if (SUCCEEDED(hr)) hr = pPersistFile->Load(lnkPath, STGM_READWRITE);
	if (FAILED(hr)) {
		pShellLink->Release();
		CoUninitialize();
		return false;
	}

	// 3. 修改目标路径
	hr = pShellLink->SetPath(newTargetPath);
	if (SUCCEEDED(hr)) {
		hr = pPersistFile->Save(lnkPath, TRUE); // 保存修改
	}

	// 4. 释放资源
	pPersistFile->Release();
	pShellLink->Release();
	CoUninitialize();
	return SUCCEEDED(hr);
}

int main()
{
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = 创建快捷方式:C:\\Users\\46043\\Desktop\\记事本.lnk" << std::endl;
		std::wcout << L"输入2 = 获取快捷方式的目标路径:C:\\Users\\46043\\Desktop\\记事本.lnk" << std::endl;
		std::wcout << L"输入3 = 修改快捷方式的目标路径:C:\\Users\\46043\\Desktop\\记事本.lnk" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			bool success = CreateShortcut(
				L"C:\\Windows\\System32\\notepad.exe",		// 目标程序
				L"C:\\Users\\46043\\Desktop\\记事本.lnk",	// 快捷方式保存路径
				L"C:\\log.txt",								// 启动参数（打开指定文件）
				L"C:\\Windows\\System32",                   // 工作目录
				L"快速打开记事本",							// 描述
				L"C:\\Windows\\System32\\notepad.exe",		// 图标路径（同目标程序）
				0											// 图标索引
			);

			if (success)
				std::wcout << L"快捷方式创建成功!" << std::endl;
			else
				std::wcout << L"快捷方式创建失败!" << std::endl;
		}
		else if (iInput == 2)
		{
			const wchar_t* shortcutPath = L"C:\\Users\\46043\\Desktop\\记事本.lnk";
			wchar_t targetPath[MAX_PATH] = { 0 };
			wchar_t szArgu[MAX_PATH] = { 0 };
			wchar_t szDir[MAX_PATH] = { 0 };
			if (GetShortcutTarget(shortcutPath, targetPath, szArgu, szDir))
				wprintf(L"快捷方式目标路径: %s\n参数: %s\n工作目录: %s\n", targetPath, szArgu, szDir);
			else
				wprintf(L"无法获取快捷方式目标\n");
		}
		else if (iInput == 3)
		{
			if (ModifyShortcutTarget(L"C:\\Users\\46043\\Desktop\\记事本.lnk", L"C:\\Windows\\System32\\calc.exe"))
				std::wcout << L"快捷方式目标修改成功!" << std::endl;
			else
				std::wcout << L"修改失败!" << std::endl;
		}
	}
	return 0;
}