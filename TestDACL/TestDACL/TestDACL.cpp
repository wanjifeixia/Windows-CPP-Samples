#include <Windows.h>
#include <Sddl.h>
#include <AclAPI.h>

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

int main(int argc, char* argv[])
{
	AddNetworkServicePermission(L"C:\\Users\\xxxx\\Desktop\\新建 文本文档.txt");
	return 0;
}