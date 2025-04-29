#include <windows.h>
#include <iostream>
#include <string>
#include <Shlwapi.h>
#include <vector>
#include <fstream>

#pragma comment(lib, "Shlwapi.lib")

// 将错误码转换为可读消息
std::wstring GetLastErrorAsString() {
	DWORD errorCode = GetLastError();
	if (errorCode == 0) {
		return L"No error";
	}

	LPWSTR buffer = nullptr;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&buffer,
		0,
		nullptr
	);

	std::wstring message(buffer);
	LocalFree(buffer);
	return message;
}

int TestReplaceFile() {
	// 定义文件路径（请根据实际路径修改）
	const wchar_t* originalFile = L"OriginalFile.txt";
	const wchar_t* replacementFile = L"ReplacementFile.txt";
	const wchar_t* backupFile = L"BackupFile.txt";

	// 步骤 1: 创建测试文件（仅用于演示）
	HANDLE hFile = CreateFileW(originalFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::wcerr << L"创建原始文件失败: " << GetLastErrorAsString() << std::endl;
		return 1;
	}
	CloseHandle(hFile);

	hFile = CreateFileW(replacementFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::wcerr << L"创建替换文件失败: " << GetLastErrorAsString() << std::endl;
		return 1;
	}
	CloseHandle(hFile);

	// 步骤 2: 调用 ReplaceFileW
	BOOL success = ReplaceFileW(
		originalFile,       // 被替换的文件路径
		replacementFile,    // 新文件路径（将替换原始文件）
		backupFile,         // 备份文件路径（可选，设为 nullptr 则不备份）
		REPLACEFILE_IGNORE_MERGE_ERRORS,  // 替换标志
		nullptr,            // 保留参数（必须为 nullptr）
		nullptr             // 保留参数（必须为 nullptr）
	);

	// 步骤 3: 检查结果
	if (!success) {
		DWORD error = GetLastError();
		std::wcerr << L"替换文件失败: " << GetLastErrorAsString() << std::endl;

		// 常见错误处理
		if (error == ERROR_FILE_NOT_FOUND) {
			std::wcerr << L"文件不存在，请检查路径是否正确。" << std::endl;
		}
		else if (error == ERROR_ACCESS_DENIED) {
			std::wcerr << L"权限不足，请以管理员身份运行程序。" << std::endl;
		}
		return 1;
	}

	std::wcout << L"文件替换成功！" << std::endl;
	return 0;
}

//匹配通配符路径
BOOL FindFiles(std::wstring& strFilePath, std::vector<std::wstring>& vtPath)
{
	std::vector<std::wstring> vtTruePath;
	auto pos = strFilePath.find(L'*');
	if (pos == std::string::npos)
		return FALSE;

	auto prePos = strFilePath.rfind(L'\\', pos);
	if (prePos == std::string::npos)
		return FALSE;
	std::wstring strPrePath = strFilePath.substr(0, prePos + 1);

	auto postPos = strFilePath.find(L'\\', pos);
	std::wstring strPostPath;
	if (postPos != std::string::npos)
		strPostPath = strFilePath.substr(postPos);

	WIN32_FIND_DATAW FindFileData;
	HANDLE hFile = FindFirstFile(strFilePath.substr(0, postPos).c_str(), &FindFileData);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!wcscmp(FindFileData.cFileName, L".") || !wcscmp(FindFileData.cFileName, L".."))
					continue;
			}

			std::wstring strTruePath = strPrePath + FindFileData.cFileName + strPostPath;
			if (PathFileExists(strTruePath.c_str()))
				vtPath.push_back(strTruePath);

		} while (FindNextFile(hFile, &FindFileData));
	}
	FindClose(hFile);
	return TRUE;
}

//删除文件夹
void DeleteFolder(const std::wstring& folderPath)
{
	std::wstring searchPath = folderPath + L"\\*.*";
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (findData.cFileName[0] == '.')
				continue;

			std::wstring filePath = folderPath + L"\\" + findData.cFileName;

			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// 如果是文件夹，则递归调用DeleteFolder函数删除子文件夹
				DeleteFolder(filePath);
			}
			else
			{
				// 如果是文件，则直接删除
				DeleteFile(filePath.c_str());

				WIN32_FIND_DATA findData1;
				HANDLE h = FindFirstFile(filePath.c_str(), &findData1);
				if (h == NULL || h == INVALID_HANDLE_VALUE)
				{
					std::wcout << L"Success to delete file: " << filePath << std::endl;
				}
				else
				{
					std::wcout << L"Failed to delete file: " << filePath << std::endl;
				}
			}
		} while (FindNextFile(hFind, &findData));

		FindClose(hFind);
	}

	// 删除空文件夹
	RemoveDirectory(folderPath.c_str());
	WIN32_FIND_DATA findData1;
	HANDLE h = FindFirstFile(folderPath.c_str(), &findData1);
	if (h == NULL || h == INVALID_HANDLE_VALUE)
	{
		std::wcout << L"Success to delete folder: " << folderPath << std::endl;
	}
	else
	{
		std::wcout << L"Failed to delete folder: " << folderPath << std::endl;
	}
}

bool BlockFile(const std::wstring& filePath) {
	// 构造文件的 Zone.Identifier 数据流路径
	std::wstring zoneIdentifierPath = filePath + L":Zone.Identifier";

	// 打开数据流文件并写入标记内容
	std::ofstream zoneFile(zoneIdentifierPath);
	if (zoneFile.is_open()) {
		// 写入 "ZoneId=3" 表示文件来自互联网
		zoneFile << "[ZoneTransfer]\nZoneId=3";
		zoneFile.close();
		return true;
	}
	else {
		std::wcerr << L"添加锁定失败，无法打开文件。" << std::endl;
		return false;
	}
}

bool UnblockFile(const std::wstring& filePath) {
	// 构造文件的 Zone.Identifier 数据流路径
	std::wstring zoneIdentifierPath = filePath + L":Zone.Identifier";

	// 使用 DeleteFile 删除数据流
	if (DeleteFile(zoneIdentifierPath.c_str())) {
		return true;
	}
	else {
		std::wcerr << L"解除锁定失败，错误代码: " << GetLastError() << std::endl;
		return false;
	}
}

int main(int argc, wchar_t* argv[])
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

		std::wcout << L"输入1 = 使用ReplaceFile替换文件" << std::endl;
		std::wcout << L"输入2 = 使用FindFirstFile通配符路径匹配 C:\\Windows\\*?????.exe*" << std::endl;
		std::wcout << L"输入3 = 删除整个文件夹 D:\\WPS Office" << std::endl;
		std::wcout << L"输入4 = 文件添加锁定标记 D:\\toolss\\Spy.exe" << std::endl;
		std::wcout << L"输入5 = 文件解除锁定标记 D:\\toolss\\Spy.exe" << std::endl;
		std::wcout << L"输入6 = 打开目录符号链接(SymbolicLink) C:\\CCCCCCCCC\\1111111" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			TestReplaceFile();
		}
		else if (iInput == 2)
		{
			WCHAR szFindDir[MAX_PATH] = { 0 };
			wcscat_s(szFindDir, L"C:\\Windows\\*?????.exe*");
			std::wstring strFindDir = szFindDir;
			std::vector<std::wstring> vtPath;
			FindFiles(strFindDir, vtPath);
			for (auto& strFile : vtPath)
			{
				std::wcout << strFile << std::endl;
			}
		}
		else if (iInput == 3)
		{
			std::wstring folderPath = L"D:\\WPS Office";
			DeleteFolder(folderPath);
		}
		else if (iInput == 4)
		{
			std::wstring filePath = L"D:\\toolss\\Spy.exe";
			if (BlockFile(filePath)) {
				std::wcout << L"文件已添加锁定标记。" << std::endl;
			}
			else {
				std::wcout << L"文件添加锁定标记失败。" << std::endl;
			}
		}
		else if (iInput == 5)
		{
			std::wstring filePath = L"D:\\toolss\\Spy.exe";
			if (UnblockFile(filePath)) {
				std::wcout << L"文件已解除锁定。" << std::endl;
			}
			else {
				std::wcout << L"文件解除锁定失败。" << std::endl;
			}
		}
		else if (iInput == 6)
		{
			HANDLE hDir = CreateFile(
				L"C:\\CCCCCCCCC\\1111111",                // 文件夹路径
				GENERIC_READ,                     // 访问权限
				FILE_SHARE_READ | FILE_SHARE_WRITE, // 共享模式
				NULL,                             // 安全属性
				OPEN_EXISTING,                   // 创建方式
				FILE_FLAG_BACKUP_SEMANTICS,     // 标志以打开文件夹
				NULL                              // 模板文件句柄
			);

			if (hDir == INVALID_HANDLE_VALUE) {
				std::cerr << "无法打开文件夹: C:\\CCCCCCCCC\\1111111 GetLastError:" << GetLastError() << std::endl;
			}
			else {
				std::cout << "文件夹已成功打开！" << std::endl;
				CloseHandle(hDir);
			}
		}
	}
	return 0;
}