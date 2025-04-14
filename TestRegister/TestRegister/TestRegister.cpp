#include <iostream>
#include <Windows.h>
#include <aclapi.h>

int TestRegEnumValue() {
	HKEY hKey;
	LONG result;

	// 打开注册表键
	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\Software\\Microsoft\\TIP\\TestResults\\36923819\\{0FACED99-D4DE-40DA-85A1-695FA3B3B4C9}", 0, KEY_ALL_ACCESS, &hKey);
	if (result != ERROR_SUCCESS) {
		std::cout << "无法打开注册表键。错误代码: " << result << std::endl;
		return 1;
	}

	DWORD index = 0;
	TCHAR valueName[256];
	DWORD valueNameSize = sizeof(valueName) / sizeof(valueName[0]);
	BYTE data[1024];
	DWORD dataSize = sizeof(data);

	while (true) {
		result = RegEnumValue(hKey, 0, valueName, &valueNameSize, nullptr, nullptr, data, &dataSize);
		if (result == ERROR_SUCCESS) {
			std::wcout << L"Value Name: " << valueName << std::endl;
			// 可以在这里使用data和dataSize来处理注册表值的数据
			auto deleteResult = RegDeleteValue(hKey, valueName);
			std::wcout << L"Value Name: " << valueName << L"; RegDeleteValue result:" << deleteResult << std::endl;
			index++;
			valueNameSize = sizeof(valueName) / sizeof(valueName[0]);
			dataSize = sizeof(data);
		}
		else if (result == ERROR_NO_MORE_ITEMS) {
			break; // 已经枚举完所有值
		}
		else {
			std::wcout << L"Failed to enumerate registry value." << std::endl;
			break;
		}
	}

	RegCloseKey(hKey);
	getchar();
	return 0;
}

void SetRegistryKeySecurity(HKEY hKey, const std::wstring& subKey) {
	// 创建一个新的安全描述符
	PSECURITY_DESCRIPTOR pSecurityDescriptor = nullptr;
	PACL pAcl = nullptr;
	SECURITY_ATTRIBUTES sa;
	DWORD dwRes;

	// 创建一个空的 DACL
	dwRes = SetEntriesInAcl(0, nullptr, nullptr, &pAcl);
	if (dwRes != ERROR_SUCCESS) {
		std::cerr << "SetEntriesInAcl failed with error " << dwRes << std::endl;
		return;
	}

	// 初始化安全描述符
	pSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (!pSecurityDescriptor) {
		std::cerr << "LocalAlloc failed with error " << GetLastError() << std::endl;
		LocalFree(pAcl);
		return;
	}

	if (!InitializeSecurityDescriptor(pSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION)) {
		std::cerr << "InitializeSecurityDescriptor failed with error " << GetLastError() << std::endl;
		LocalFree(pSecurityDescriptor);
		LocalFree(pAcl);
		return;
	}

	// 将 DACL 添加到安全描述符
	if (!SetSecurityDescriptorDacl(pSecurityDescriptor, TRUE, pAcl, FALSE)) {
		std::cerr << "SetSecurityDescriptorDacl failed with error " << GetLastError() << std::endl;
		LocalFree(pSecurityDescriptor);
		LocalFree(pAcl);
		return;
	}

	// 设置安全属性
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSecurityDescriptor;
	sa.bInheritHandle = FALSE;

	// 打开注册表项
	HKEY hSubKey;
	LONG lResult = RegOpenKeyEx(hKey, subKey.c_str(), 0, WRITE_DAC, &hSubKey);
	if (lResult != ERROR_SUCCESS) {
		std::cerr << "RegOpenKeyEx failed with error " << lResult << std::endl;
		LocalFree(pSecurityDescriptor);
		LocalFree(pAcl);
		return;
	}

	// 设置注册表项的安全描述符
	dwRes = RegSetKeySecurity(hSubKey, DACL_SECURITY_INFORMATION, pSecurityDescriptor);
	if (dwRes != ERROR_SUCCESS) {
		std::cerr << "RegSetKeySecurity failed with error " << dwRes << std::endl;
	}
	else {
		std::wcout << L"Security descriptor set successfully for " << subKey << std::endl;
	}

	// 清理
	RegCloseKey(hSubKey);
	LocalFree(pSecurityDescriptor);
	LocalFree(pAcl);
}

// 辅助函数：将HKEY转换为根键名称
const wchar_t* GetRootKeyName(HKEY rootKey) {
	if (rootKey == HKEY_CLASSES_ROOT) return L"HKEY_CLASSES_ROOT";
	if (rootKey == HKEY_CURRENT_USER) return L"HKEY_CURRENT_USER";
	if (rootKey == HKEY_LOCAL_MACHINE) return L"HKEY_LOCAL_MACHINE";
	if (rootKey == HKEY_USERS) return L"HKEY_USERS";
	if (rootKey == HKEY_CURRENT_CONFIG) return L"HKEY_CURRENT_CONFIG";
	return L"UNKNOWN_ROOT_KEY";
}

// 统一错误处理函数
void PrintRegError(LONG errCode, const std::wstring& keyPath) {
	std::wcerr << L"操作注册表键 [" << keyPath << L"] 失败，错误代码: 0x"
		<< std::hex << errCode << L" (" << errCode << L") - ";

	switch (errCode) {
	case ERROR_FILE_NOT_FOUND:
		std::wcerr << L"注册表键不存在";
		break;
	case ERROR_ACCESS_DENIED:
		std::wcerr << L"权限不足（请以管理员身份运行）";
		break;
	case ERROR_INVALID_HANDLE:
		std::wcerr << L"无效的句柄";
		break;
	default:
		std::wcerr << L"未知错误";
		break;
	}
	std::wcerr << std::endl;
}

void TestRegCreateKeyEx(HKEY rootKey, const std::wstring& subKeyPath, DWORD ulOpsWOW64KEY = 0) {
	HKEY hKey;
	DWORD dwDisposition;
	LSTATUS status = RegCreateKeyExW(
		rootKey,
		subKeyPath.c_str(),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS | ulOpsWOW64KEY,
		NULL,
		&hKey,
		&dwDisposition);

	if (status == ERROR_SUCCESS) {
		std::wcerr << L"注册表项创建成功。" << std::endl;
		RegCloseKey(hKey);
	}
	else {
		std::wstring fullPath = std::wstring(GetRootKeyName(rootKey)) + L"\\" + subKeyPath;
		PrintRegError(status, fullPath);
	}
}

void TestRegRenameKey(HKEY rootKey, const std::wstring& oldSubKeyPath, const std::wstring& newKeyName) {
	auto status = ::RegRenameKey(rootKey, oldSubKeyPath.c_str(), newKeyName.c_str());
	if (status != ERROR_SUCCESS) {
		std::wstring fullPath = std::wstring(GetRootKeyName(rootKey)) + L"\\" + oldSubKeyPath;
		PrintRegError(status, fullPath);
		return;
	}
	std::wcout << L"重命名注册表键成功" << std::endl;
}

int TestRegOpenKeyExW(HKEY rootKey, const std::wstring& rootSubPath, const std::wstring& subSubPath, DWORD ulOpsWOW64KEY = 0) {
	HKEY hRootKey = nullptr;
	LONG result = RegOpenKeyExW(
		rootKey,
		rootSubPath.c_str(),
		0,
		KEY_ALL_ACCESS | ulOpsWOW64KEY,
		&hRootKey
	);

	if (result != ERROR_SUCCESS) {
		std::wstring fullPath = std::wstring(GetRootKeyName(rootKey)) + L"\\" + rootSubPath;
		PrintRegError(result, fullPath);
		return result;
	}

	std::wcout << L"成功打开注册表键: " << GetRootKeyName(rootKey) << L"\\" << rootSubPath << std::endl;

	HKEY hSubKey = nullptr;
	result = RegOpenKeyExW(
		hRootKey,
		subSubPath.c_str(),
		0,
		KEY_ALL_ACCESS,
		&hSubKey
	);

	RegCloseKey(hRootKey);

	if (result == ERROR_SUCCESS) {
		std::wstring fullSubPath = std::wstring(GetRootKeyName(rootKey)) + L"\\" + rootSubPath + L"\\" + subSubPath;
		std::wcout << L"成功打开注册表键: " << fullSubPath << std::endl;
		RegCloseKey(hSubKey);
		return ERROR_SUCCESS;
	}

	std::wstring fullSubPath = std::wstring(GetRootKeyName(rootKey)) + L"\\" + rootSubPath + L"\\" + subSubPath;
	PrintRegError(result, fullSubPath);
	return result;
}

// 删除注册表键（包含递归删除）
bool TestRegDeleteKey(HKEY rootKey, const std::wstring& subKeyPath, bool recursive = false, DWORD ulOpsWOW64KEY = 0)
{
	const std::wstring fullPath = std::wstring(GetRootKeyName(rootKey)) + L"\\" + subKeyPath;

	// 递归删除需要特殊处理（Vista+系统可用RegDeleteTree）
	if (recursive) {
		// 递归删除需要分两步操作
		HKEY hParentKey;
		std::wstring parentPath = subKeyPath.substr(0, subKeyPath.find_last_of(L'\\'));
		std::wstring keyName = subKeyPath.substr(subKeyPath.find_last_of(L'\\') + 1);

		// 1. 打开父键
		LSTATUS status = RegOpenKeyExW(
			rootKey,
			parentPath.empty() ? nullptr : parentPath.c_str(),
			0,
			KEY_ALL_ACCESS | ulOpsWOW64KEY,
			&hParentKey
		);

		if (status != ERROR_SUCCESS) {
			PrintRegError(status, GetRootKeyName(rootKey) + std::wstring(L"\\") + parentPath);
			return false;
		}

		// 2. 递归删除子项
		status = RegDeleteTreeW(hParentKey, keyName.c_str());
		RegCloseKey(hParentKey);

		if (status == ERROR_SUCCESS) {
			std::wcout << L"成功删除注册表键: " << fullPath << std::endl;
			return true;
		}

		PrintRegError(status, fullPath);
		return false;
	}

	// 非递归删除（仅能删除空键）
	LSTATUS status = RegDeleteKeyExW(
		rootKey,
		subKeyPath.c_str(),
		KEY_WOW64_64KEY,  // 显式指定64位视图
		0
	);

	if (status == ERROR_SUCCESS) {
		std::wcout << L"成功删除注册表键: " << fullPath << std::endl;
		return true;
	}

	// 处理旧系统兼容性（Windows 2000/XP）
	if (status == ERROR_CALL_NOT_IMPLEMENTED) {
		status = RegDeleteKeyW(rootKey, subKeyPath.c_str());
		if (status == ERROR_SUCCESS) {
			std::wcout << L"成功删除注册表键: " << fullPath << std::endl;
			return true;
		}
	}

	PrintRegError(status, fullPath);
	return false;
}

// 需要包含Windows.h头文件
const wchar_t* GetProcessBitness() {
#ifdef _WIN64
	return L"64-bit";
#else
	BOOL isWow64 = FALSE;
	if (IsWow64Process(GetCurrentProcess(), &isWow64)) {
		return isWow64 ? L"32-bit (WOW64)" : L"32-bit";
	}
	return L"32-bit";
#endif
}

int main()
{
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));

	//TestRegEnumValue();

	// 设置 HKEY_CURRENT_USER\Software\MyApp 的安全描述符
	//SetRegistryKeySecurity(HKEY_CURRENT_USER, L"Software\\TestBBB");

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"当前进程位数：[" << GetProcessBitness() << L" 进程]" << std::endl;
		std::wcout << L"输入1 = RegCreateKeyEx创建注册表HKEY_CURRENT_USER\\Software\\TestAAA\\TestBBB" << std::endl;
		std::wcout << L"输入2 = RegRenameKey重命名注册表HKEY_CURRENT_USER\\Software\\TestAAA\\TestBBB -> TestCCC" << std::endl;
		std::wcout << L"输入3 = RegCreateKeyEx创建注册表HKEY_LOCAL_MACHINE\\Software\\TestAAA\\TestBBB" << std::endl;
		std::wcout << L"输入4 = RegOpenKeyEx先打开HKEY_LOCAL_MACHINE\\Software\\TestAAA，再打开子键TestBBB" << std::endl;
		std::wcout << L"输入5 = RegDeleteTree递归删除HKEY_CURRENT_USER\\Software\\TestAAA和HKEY_LOCAL_MACHINE\\Software\\TestAAA" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			TestRegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\TestAAA\\TestBBB", KEY_WOW64_64KEY);
		}
		else if (iInput == 2)
		{
			TestRegRenameKey(HKEY_CURRENT_USER, L"Software\\TestAAA\\TestBBB", L"TestCCC");
		}
		else if (iInput == 3)
		{
			TestRegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Software\\TestAAA\\TestBBB", KEY_WOW64_64KEY);
		}
		else if (iInput == 4)
		{
			TestRegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\TestAAA", L"TestBBB", KEY_WOW64_64KEY);
		}
		else if (iInput == 5)
		{
			//启用递归删除
			TestRegDeleteKey(HKEY_CURRENT_USER, L"Software\\TestAAA", true, KEY_WOW64_64KEY);
			TestRegDeleteKey(HKEY_LOCAL_MACHINE, L"Software\\TestAAA", true, KEY_WOW64_64KEY);
		}
	}

	return 0;
}