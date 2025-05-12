#include <windows.h>

#include <msctf.h>
#include <tchar.h>
#include <comutil.h>
#include <stdio.h>
#include <string>
#include <memory>
using namespace std;

#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")

std::string GuidToString(const GUID& guid)
{
	char buf[64] = { 0 };
	sprintf_s(buf, sizeof(buf),
		"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	return std::string(buf);
}

int _tmain(int argc, TCHAR* argv[])
{
	CoInitialize(0);
	HRESULT hr = S_OK;

	//����Profiles�ӿ�
	ITfInputProcessorProfiles* pProfiles;
	hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITfInputProcessorProfiles,
		(LPVOID*)&pProfiles);

	if (SUCCEEDED(hr))
	{
		//ö���������뷨��
		IEnumTfLanguageProfiles* pEnumProf = 0;
		hr = pProfiles->EnumLanguageProfiles(0x804, &pEnumProf);
		if (SUCCEEDED(hr) && pEnumProf)
		{
			//��ʵproArr����Ӧ��д�� &proArr[0]����Ϊ����ֻ��Ҫһ��TF_LANGUAGEPROFILE���������ң�proArr[1]��û�õ�����
			TF_LANGUAGEPROFILE proArr[2];
			ULONG feOut = 0;
			while (S_OK == pEnumProf->Next(1, proArr, &feOut))
			{
				//��ȡ����
				BSTR bstrDest;
				hr = pProfiles->GetLanguageProfileDescription(proArr[0].clsid, 0x804, proArr[0].guidProfile, &bstrDest);

				BOOL bEnable = false;
				hr = pProfiles->IsEnabledLanguageProfile(proArr[0].clsid, 0x804, proArr[0].guidProfile, &bEnable);
				if (SUCCEEDED(hr))
				{
					printf("[clsid%s][guid%s][%s]����״̬:%d\n", GuidToString(proArr[0].clsid).c_str(), GuidToString(proArr[0].guidProfile).c_str(), _com_util::ConvertBSTRToString(bstrDest), bEnable);
				}

				//�����ѽ��õ����뷨
				if (!bEnable)
				{
					hr = pProfiles->EnableLanguageProfile(proArr[0].clsid, 0x804, proArr[0].guidProfile, !bEnable);
					if (SUCCEEDED(hr))
					{
						printf("%s�ɹ�\n\n", bEnable ? "����" : "����");
					}
				}

				SysFreeString(bstrDest);
			}
		}

		pProfiles->Release();
	}

	CoUninitialize();
	getchar();
	return 0;
}