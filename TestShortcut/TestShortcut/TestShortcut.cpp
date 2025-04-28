#include <windows.h>
#include <shlobj.h>
#include <wchar.h>
#include <stdio.h>
#include <xlocale>
#include <iostream>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// 获取快捷方式目标路径的函数
bool GetShortcutTarget(const wchar_t* shortcutPath, wchar_t* targetPath, size_t bufferSize) {
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
	hr = pShellLink->GetPath(targetPath, bufferSize, nullptr, SLGP_UNCPRIORITY);
	if (FAILED(hr)) {
		wprintf(L"获取路径失败: 0x%08X\n", hr);
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

int main() {
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = 获取快捷方式的目标路径:C:\\Users\\Public\\Desktop\\Foxmail.lnk" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			const wchar_t* shortcutPath = L"C:\\Users\\Public\\Desktop\\Foxmail.lnk";
			wchar_t targetPath[MAX_PATH] = { 0 };
			if (GetShortcutTarget(shortcutPath, targetPath, MAX_PATH))
				wprintf(L"快捷方式目标路径: %s\n", targetPath);
			else
				wprintf(L"无法获取快捷方式目标\n");
		}
		else if (iInput == 2)
		{

		}
	}
	return 0;
}