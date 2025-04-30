#include <Windows.h>
#include <Sddl.h>
#include <AclAPI.h>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include <iomanip>
#include <sstream>
#include <tchar.h>

BOOL CheckNetworkServicePermission(const WCHAR* filePath) {
	BOOL bHasNetworkServicePermission = FALSE;
	PACL pDacl = NULL;

	// 获取文件(夹)安全对象的DACL列表
	if (ERROR_SUCCESS == GetNamedSecurityInfo(filePath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pDacl, NULL, NULL)) {
		EXPLICIT_ACCESS ea;
		WCHAR* pszNetworkService = (WCHAR*)L"NETWORK SERVICE";

		// 初始化ea结构
		BuildExplicitAccessWithName(&ea, pszNetworkService, 0, SET_ACCESS, NO_INHERITANCE);

		// 检查NETWORK SERVICE是否在DACL中
		ULONG count = 0;
		PACL pNewDacl = NULL;
		if (ERROR_SUCCESS == SetEntriesInAcl(1, &ea, pDacl, &pNewDacl)) {
			if (ERROR_SUCCESS == GetExplicitEntriesFromAcl(pNewDacl, &count, NULL)) {
				// 如果count大于0，表示NETWORK SERVICE在DACL中
				bHasNetworkServicePermission = (count > 0);
			}
			LocalFree(pNewDacl);
		}
	}

	return bHasNetworkServicePermission;
}

BOOL AddNetworkServicePermission(const WCHAR* filePath) {
	// 首先检查文件是否已经有NETWORK SERVICE权限
	if (CheckNetworkServicePermission(filePath)) {
		// 文件已经有NETWORK SERVICE权限，无需添加
		return TRUE;
	}

	// 获取文件(夹)安全对象的DACL列表
	PACL pDacl = NULL;
	if (ERROR_SUCCESS != GetNamedSecurityInfo(filePath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pDacl, NULL, NULL)) {
		// 获取DACL失败
		return FALSE;
	}

	EXPLICIT_ACCESS ea;
	WCHAR* pszNetworkService = (WCHAR*)L"NETWORK SERVICE";

	// 初始化ea结构，赋予NETWORK SERVICE全部访问权限
	BuildExplicitAccessWithName(&ea, pszNetworkService, GENERIC_ALL, GRANT_ACCESS, SUB_CONTAINERS_AND_OBJECTS_INHERIT);

	// 将NETWORK SERVICE的权限加入到DACL中
	PACL pNewDacl = NULL;
	if (ERROR_SUCCESS != SetEntriesInAcl(1, &ea, pDacl, &pNewDacl)) {
		// 设置新的DACL失败
		LocalFree(pDacl);
		return FALSE;
	}

	// 设置文件(夹)安全对象的DACL列表
	if (ERROR_SUCCESS != SetNamedSecurityInfo((LPWSTR)filePath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDacl, NULL)) {
		// 设置安全信息失败
		LocalFree(pNewDacl);
		return FALSE;
	}

	// 释放内存
	LocalFree(pNewDacl);

	return TRUE;
}

// 将SID转换为字符串并输出
void PrintSidInfo(PSID pSid) {
	TCHAR szName[256] = { 0 }, szDomain[256] = { 0 };
	DWORD cchName = 256, cchDomain = 256;
	SID_NAME_USE sidType;

	// 尝试获取账户名称
	BOOL bLookup = LookupAccountSid(
		nullptr,
		pSid,
		szName,
		&cchName,
		szDomain,
		&cchDomain,
		&sidType
	);

	LPTSTR szSid = nullptr;
	ConvertSidToStringSid(pSid, &szSid); // 始终获取SID字符串

	if (bLookup) {
		_tprintf(_T("账户: %s\\%s\n"), szDomain, szName);
	}
	else {
		_tprintf(_T("账户名解析失败\n"));
	}
	_tprintf(_T("SID: %s\n"), szSid);
	LocalFree(szSid);
}

// 解析ACE类型
const TCHAR* GetAceTypeName(BYTE aceType) {
	switch (aceType) {
	case ACCESS_ALLOWED_ACE_TYPE:        return _T("允许");
	case ACCESS_DENIED_ACE_TYPE:         return _T("拒绝");
	case SYSTEM_AUDIT_ACE_TYPE:          return _T("审核");
	case ACCESS_ALLOWED_OBJECT_ACE_TYPE: return _T("对象允许");
	case ACCESS_DENIED_OBJECT_ACE_TYPE:  return _T("对象拒绝");
	default:                             return _T("未知");
	}
}

// 解析继承标志
void PrintInheritanceFlags(BYTE aceFlags) {
	_tprintf(_T("继承标志: "));
	if (aceFlags & OBJECT_INHERIT_ACE)         _tprintf(_T("对象继承 "));
	if (aceFlags & CONTAINER_INHERIT_ACE)       _tprintf(_T("容器继承 "));
	if (aceFlags & NO_PROPAGATE_INHERIT_ACE)    _tprintf(_T("不传播 "));
	if (aceFlags & INHERIT_ONLY_ACE)            _tprintf(_T("仅继承 "));
	if (aceFlags & INHERITED_ACE)               _tprintf(_T("已继承 "));
	_tprintf(_T("\n"));
}

// 将访问掩码转换为可读权限字符串（更详细）
void PrintAccessMask(DWORD dwMask) {
	_tprintf(_T("访问掩码 (0x%08X):\n"), dwMask);
	_tprintf(_T("\t特定权限: "));
	if (dwMask & FILE_READ_DATA)        _tprintf(_T("读取数据 "));
	if (dwMask & FILE_WRITE_DATA)       _tprintf(_T("写入数据 "));
	if (dwMask & FILE_APPEND_DATA)      _tprintf(_T("追加数据 "));
	if (dwMask & FILE_READ_EA)          _tprintf(_T("读扩展属性 "));
	if (dwMask & FILE_WRITE_EA)         _tprintf(_T("写扩展属性 "));
	if (dwMask & FILE_EXECUTE)          _tprintf(_T("执行 "));
	if (dwMask & FILE_DELETE_CHILD)     _tprintf(_T("删除子对象 "));
	if (dwMask & FILE_READ_ATTRIBUTES)  _tprintf(_T("读属性 "));
	if (dwMask & FILE_WRITE_ATTRIBUTES) _tprintf(_T("写属性 "));
	_tprintf(_T("\n\t通用权限: "));
	if (dwMask & GENERIC_READ)          _tprintf(_T("泛读 "));
	if (dwMask & GENERIC_WRITE)         _tprintf(_T("泛写 "));
	if (dwMask & GENERIC_EXECUTE)       _tprintf(_T("泛执行 "));
	if (dwMask & GENERIC_ALL)           _tprintf(_T("完全控制 "));
	_tprintf(_T("\n\t标准权限: "));
	if (dwMask & DELETE)                _tprintf(_T("删除 "));
	if (dwMask & READ_CONTROL)          _tprintf(_T("读安全描述符 "));
	if (dwMask & WRITE_DAC)             _tprintf(_T("修改DACL "));
	if (dwMask & WRITE_OWNER)           _tprintf(_T("取得所有权 "));
	if (dwMask & SYNCHRONIZE)           _tprintf(_T("同步 "));
	_tprintf(_T("\n"));
}

// 枚举文件权限的主函数
void EnumerateFilePermissions(LPCTSTR lpszFileName) {
	PACL pDacl = nullptr;
	PSECURITY_DESCRIPTOR pSD = nullptr;

	// 获取文件的安全描述符
	DWORD dwRes = GetNamedSecurityInfo(
		lpszFileName,
		SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION,
		nullptr,
		nullptr,
		&pDacl,
		nullptr,
		&pSD
	);

	if (dwRes != ERROR_SUCCESS) {
		_tprintf(_T("错误: 无法获取安全信息 (代码 %d)\n"), dwRes);
		return;
	}

	if (pDacl == nullptr) {
		_tprintf(_T("警告: 文件没有DACL（所有人完全访问）\n"));
		LocalFree(pSD);
		return;
	}

	ACL_SIZE_INFORMATION aclSizeInfo;
	if (!GetAclInformation(pDacl, &aclSizeInfo, sizeof(aclSizeInfo), AclSizeInformation)) {
		_tprintf(_T("错误: 无法获取ACL信息 (代码 %d)\n"), GetLastError());
		LocalFree(pSD);
		return;
	}

	_tprintf(_T("文件: %s\n"), lpszFileName);
	_tprintf(_T("发现 %d 个ACE项\n\n"), aclSizeInfo.AceCount);

	for (DWORD i = 0; i < aclSizeInfo.AceCount; i++) {
		LPVOID pAce = nullptr;
		if (!GetAce(pDacl, i, &pAce)) {
			_tprintf(_T("错误: 无法读取ACE[%d] (代码 %d)\n"), i, GetLastError());
			continue;
		}

		ACE_HEADER* aceHeader = (ACE_HEADER*)pAce;
		_tprintf(_T("=== ACE项 %d ===\n"), i + 1);
		_tprintf(_T("类型: %s\n"), GetAceTypeName(aceHeader->AceType));
		PrintInheritanceFlags(aceHeader->AceFlags);

		if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE ||
			aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
		{
			ACCESS_ALLOWED_ACE* pAceData = (ACCESS_ALLOWED_ACE*)pAce;
			PSID pSid = (PSID)&pAceData->SidStart;
			PrintSidInfo(pSid);
			PrintAccessMask(pAceData->Mask);
		}
		else
		{
			_tprintf(_T("警告: 未处理的ACE类型 (0x%02X)\n"), aceHeader->AceType);
		}
		_tprintf(_T("\n"));
	}

	LocalFree(pSD);
}

int main(int argc, char* argv[])
{
	std::locale::global(std::locale(""));
	std::wcout.imbue(std::locale());

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = 添加Network Service权限: C:\\Users\\xxxx\\Desktop\\新建 文本文档.txt" << std::endl;
		std::wcout << L"输入2 = 枚举文件的权限列表: C:\\ProgramData\\Microsoft\\Windows\\WER\\ReportQueue" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			AddNetworkServicePermission(L"C:\\Users\\xxxx\\Desktop\\新建 文本文档.txt");
		}
		else if (iInput == 2)
		{
			EnumerateFilePermissions(L"C:\\ProgramData\\Microsoft\\Windows\\WER\\ReportQueue");
		}
	}
	return 0;
}