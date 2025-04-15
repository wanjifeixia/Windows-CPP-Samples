// TestCom.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
#include <iostream>
#include <combaseapi.h>
#include <comdef.h>
#include <shobjidl.h>    // IFileOperation 接口
#include <shlwapi.h>     // 路径处理函数
#include <fstream>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Shlwapi.lib")

// 辅助函数：获取错误描述
std::wstring GetErrorMessage(HRESULT hr) {
	_com_error err(hr);
	return err.ErrorMessage();
}

//通过 ProgID 创建 COM 对象
int MyCoCreateInstanceByProgID(LPCWSTR lpProgID)
{
	// 初始化 COM 库
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		std::wcerr << L"CoInitializeEx失败\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
		return 1;
	}

	CLSID clsid;
	// 将 ProgID 转换为 CLSID
	hr = CLSIDFromProgID(lpProgID, &clsid);
	if (SUCCEEDED(hr)) {
		IDispatch* pDispatch = nullptr;
		// 创建 COM 对象实例（需指定 CLSCTX_LOCAL_SERVER 以启动本地服务器）
		hr = CoCreateInstance(clsid, NULL, CLSCTX_LOCAL_SERVER, IID_IDispatch, (void**)&pDispatch);
		if (SUCCEEDED(hr)) {
			std::wcout << L"COM对象创建成功:" << lpProgID << std::endl;
			pDispatch->Release();
		}
		else {
			std::wcerr << L"COM对象创建失败:" << lpProgID << L"\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
		}
	}
	else {
		std::wcerr << L"CLSIDFromProgID失败:" << lpProgID << L"\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
	}

	CoUninitialize();
	return 0;
}

// 获取当前程序所在目录
std::wstring GetCurrentDirectoryPath() {
	WCHAR path[MAX_PATH];
	GetModuleFileNameW(nullptr, path, MAX_PATH);
	PathRemoveFileSpecW(path);
	return path;
}

// 创建测试文件
bool CreateTestFile(const std::wstring& filePath) {
	std::ofstream file(filePath);
	if (!file.is_open()) return false;

	file << "This is a test file created by the program.\n";
	file.close();
	return true;
}

// 创建 IShellItem 辅助函数
HRESULT CreateShellItem(PCWSTR pszPath, IShellItem** ppItem) {
	return SHCreateItemFromParsingName(pszPath, nullptr, IID_PPV_ARGS(ppItem));
}

// 初始化 COM 和 IFileOperation
HRESULT InitializeFileOperation(IFileOperation** ppFileOp) {
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		hr = CoCreateInstance(
			CLSID_FileOperation,
			nullptr,
			CLSCTX_ALL,
			IID_PPV_ARGS(ppFileOp)
		);
	}
	return hr;
}

// 文件复制操作
HRESULT CopyFileWithProgress(PCWSTR srcPath, PCWSTR destDir, PCWSTR newName = nullptr) {
	IFileOperation* pFileOp = nullptr;
	IShellItem* pSrcItem = nullptr;
	IShellItem* pDestDir = nullptr;

	HRESULT hr = InitializeFileOperation(&pFileOp);
	if (SUCCEEDED(hr)) {
		// 设置操作选项（允许撤销、不显示确认对话框）
		pFileOp->SetOperationFlags(FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR);

		// 创建源文件项
		hr = CreateShellItem(srcPath, &pSrcItem);
		if (SUCCEEDED(hr)) {
			// 创建目标目录项
			hr = CreateShellItem(destDir, &pDestDir);
			if (SUCCEEDED(hr)) {
				// 添加复制操作
				hr = pFileOp->CopyItem(pSrcItem, pDestDir, newName, nullptr);
				if (SUCCEEDED(hr)) {
					// 执行操作
					hr = pFileOp->PerformOperations();
				}
			}
		}
	}

	// 清理资源
	if (pDestDir) pDestDir->Release();
	if (pSrcItem) pSrcItem->Release();
	if (pFileOp) pFileOp->Release();
	CoUninitialize();
	return hr;
}

// 文件移动操作（参数与复制类似）
HRESULT MoveFileWithProgress(PCWSTR srcPath, PCWSTR destDir, PCWSTR newName = nullptr) {
	IFileOperation* pFileOp = nullptr;
	IShellItem* pSrcItem = nullptr;
	IShellItem* pDestDir = nullptr;

	HRESULT hr = InitializeFileOperation(&pFileOp);
	if (SUCCEEDED(hr)) {
		// 设置操作选项（允许撤销、不显示确认对话框）
		pFileOp->SetOperationFlags(FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR);

		// 创建源文件项
		hr = CreateShellItem(srcPath, &pSrcItem);
		if (SUCCEEDED(hr)) {
			// 创建目标目录项
			hr = CreateShellItem(destDir, &pDestDir);
			if (SUCCEEDED(hr)) {
				// 添加复制操作
				hr = pFileOp->MoveItem(pSrcItem, pDestDir, newName, nullptr);
				if (SUCCEEDED(hr)) {
					// 执行操作
					hr = pFileOp->PerformOperations();
				}
			}
		}
	}

	// 清理资源
	if (pDestDir) pDestDir->Release();
	if (pSrcItem) pSrcItem->Release();
	if (pFileOp) pFileOp->Release();
	CoUninitialize();
	return hr;
}

// 文件重命名操作
HRESULT RenameFileWithProgress(PCWSTR filePath, PCWSTR newName) {
	IFileOperation* pFileOp = nullptr;
	IShellItem* pItem = nullptr;

	HRESULT hr = InitializeFileOperation(&pFileOp);
	if (SUCCEEDED(hr)) {
		pFileOp->SetOperationFlags(FOF_ALLOWUNDO | FOF_RENAMEONCOLLISION);

		hr = CreateShellItem(filePath, &pItem);
		if (SUCCEEDED(hr)) {
			hr = pFileOp->RenameItem(pItem, newName, nullptr);
			if (SUCCEEDED(hr)) {
				// 执行操作
				hr = pFileOp->PerformOperations();
			}
		}
	}

	if (pItem) pItem->Release();
	if (pFileOp) pFileOp->Release();
	CoUninitialize();
	return hr;
}

int main()
{
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));
	//显式设置宽字符流的本地化
	std::wcout.imbue(std::locale(""));
	std::wcerr.imbue(std::locale(""));

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = CoInitialize(NULL)" << std::endl;
		std::wcout << L"输入2 = CoInitializeEx(NULL, COINIT_MULTITHREADED)" << std::endl;
		std::wcout << L"输入3 = CoUninitialize()" << std::endl;
		std::wcout << L"输入4 = 通过ProgID创建COM对象:GoogleUpdate.Update3WebMachine.1.0" << std::endl;
		std::wcout << L"输入5 = 通过ProgID创建COM对象:GoogleUpdate.ProcessLauncher.1.0" << std::endl;
		std::wcout << L"输入6 = IFileOperation复制" << std::endl;
		std::wcout << L"输入7 = IFileOperation移动" << std::endl;
		std::wcout << L"输入8 = IFileOperation重命名" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			HRESULT hr = CoInitialize(NULL);
			if (FAILED(hr))
				std::wcerr << L"CoInitialize失败\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
			else if (hr == S_FALSE)
				std::wcerr << L"CoInitialize成功\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: COM 库已在此线程上初始化" << std::endl;
			else
				std::wcerr << L"CoInitialize成功\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
		}
		else if (iInput == 2)
		{
			HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
			if (FAILED(hr))
				std::wcerr << L"CoInitializeEx失败\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
			else if (hr == S_FALSE)
				std::wcerr << L"CoInitializeEx成功\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: COM 库已在此线程上初始化" << std::endl;
			else
				std::wcerr << L"CoInitializeEx成功\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
		}
		else if (iInput == 3)
		{
			CoUninitialize();
		}
		else if (iInput == 4)
		{
			MyCoCreateInstanceByProgID(L"GoogleUpdate.Update3WebMachine.1.0");
		}
		else if (iInput == 5)
		{
			MyCoCreateInstanceByProgID(L"GoogleUpdate.ProcessLauncher.1.0");
		}
		else if (iInput == 6)
		{
			std::wstring currentDir = GetCurrentDirectoryPath();
			std::wstring srcPath = currentDir + L"\\source.txt";
			CreateTestFile(srcPath);
			HRESULT hr = CopyFileWithProgress(srcPath.c_str(), currentDir.c_str(), L"dest.txt");
			if (SUCCEEDED(hr))
				std::wcout << L"CopyFileWithProgress成功" << std::endl;
			else
				std::wcerr << L"CopyFileWithProgress失败\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
		}
		else if (iInput == 7)
		{
			std::wstring currentDir = GetCurrentDirectoryPath();
			std::wstring srcPath = currentDir + L"\\source.txt";
			CreateTestFile(srcPath);
			HRESULT hr = MoveFileWithProgress(srcPath.c_str(), currentDir.c_str(), L"dest.txt");
			if (SUCCEEDED(hr))
				std::wcout << L"MoveFileWithProgress成功" << std::endl;
			else
				std::wcerr << L"MoveFileWithProgress失败\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
		}
		else if (iInput == 8)
		{
			std::wstring currentDir = GetCurrentDirectoryPath();
			std::wstring srcPath = currentDir + L"\\source.txt";
			CreateTestFile(srcPath);
			HRESULT hr = RenameFileWithProgress(srcPath.c_str(), L"dest.txt");
			if (SUCCEEDED(hr))
				std::wcout << L"RenameFileWithProgress成功" << std::endl;
			else
				std::wcerr << L"RenameFileWithProgress失败\t错误码HRESULT: 0x" << std::hex << hr << L"\t错误描述: " << GetErrorMessage(hr) << std::endl;
		}
	}
	return 1;
}