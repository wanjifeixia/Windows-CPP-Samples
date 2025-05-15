#include <Windows.h>
#include <wincred.h>
#include <iostream>

#pragma comment(lib, "credui.lib")
#pragma comment(lib, "mpr.lib")

// 凭据结构体
struct CredentialData {
	std::wstring targetName;
	std::wstring username;
	std::wstring password;
};

bool saveCredential(const CredentialData& credentialData) {
	CREDENTIAL cred = {};
	cred.Type = CRED_TYPE_DOMAIN_PASSWORD;
	cred.TargetName = const_cast<wchar_t*>(credentialData.targetName.c_str());
	cred.UserName = const_cast<wchar_t*>(credentialData.username.c_str());
	cred.CredentialBlobSize = static_cast<DWORD>(credentialData.password.size() * sizeof(wchar_t));
	cred.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(credentialData.password.c_str()));
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE; // 可以根据实际需求选择存储位置

	return CredWrite(&cred, 0);
}

bool readCredential(const std::wstring& targetName, CredentialData& credentialData)
{
	CREDENTIAL* pCred;
	if (CredRead(targetName.c_str(), CRED_TYPE_DOMAIN_PASSWORD, 0, &pCred))
	{
		// 读取凭据成功
		credentialData.targetName = pCred->TargetName;
		credentialData.username = pCred->UserName;
		credentialData.password = std::wstring(reinterpret_cast<wchar_t*>(pCred->CredentialBlob), pCred->CredentialBlobSize / sizeof(wchar_t));

		CredFree(pCred);
		return true;
	}

	return false;
}

int main()
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

		std::wcout << L"输入1 = 读取凭据192.168.207.130" << std::endl;
		std::wcout << L"输入2 = 写入凭据192.168.207.130" << std::endl;
		std::wcout << L"输入3 = 枚举系统中的凭据" << std::endl;
		std::wcout << L"输入4 = 删除凭据192.168.207.130" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			CredentialData readCredentialData;
			if (readCredential(L"192.168.207.130", readCredentialData)) {
				std::wcout << L"读取凭据成功 凭据：" << readCredentialData.targetName;
				std::wcout << L"\t用户名：" << readCredentialData.username;
				std::wcout << L"\t密码：" << readCredentialData.password << std::endl;
			}
			else {
				std::cout << "未找到凭据或读取凭据失败！" << std::endl;
			}
		}
		else if (iInput == 2)
		{
			CredentialData credentialData;
			credentialData.targetName = L"192.168.207.130";
			credentialData.username = L"46043";
			credentialData.password = L"1qaz";

			if (saveCredential(credentialData))
				std::cout << "凭据保存成功！" << std::endl;
			else
				std::cout << "凭据保存失败！" << std::endl;
		}
		else if (iInput == 3)
		{
			// 列举系统中的凭据
			PCREDENTIAL* ppCredentials = nullptr;
			DWORD count = 0;
			BOOL result = CredEnumerate(nullptr, 0, &count, &ppCredentials);
			if (result)
			{
				std::wcout << L"系统中的凭据：" << std::endl;
				for (DWORD i = 0; i < count; ++i)
				{
					if (ppCredentials[i]->Type != CRED_TYPE_DOMAIN_PASSWORD)
						continue;

					std::wcout << L"凭据名称: " << ppCredentials[i]->TargetName << L"\t用户名:" << ppCredentials[i]->UserName << std::endl;
				}
				CredFree(ppCredentials);
			}
			else
			{
				DWORD errorCode = GetLastError();
				std::cout << "凭据列举失败，错误代码: " << errorCode << std::endl;
			}
		}
		else if (iInput == 4)
		{
			// 断开与共享文件夹的连接
			WNetCancelConnection2W(L"\\\\192.168.207.130", CONNECT_UPDATE_PROFILE, TRUE);
			WNetCancelConnection2W(L"\\\\192.168.207.130\\IPC$", CONNECT_UPDATE_PROFILE, TRUE);

			//清理Windows凭据
			if (CredDeleteW(L"192.168.207.130", CRED_TYPE_DOMAIN_PASSWORD, 0))
				std::cout << "凭据删除成功！" << std::endl;
			else
				std::cout << "凭据删除失败！" << std::endl;
		}
	}
}