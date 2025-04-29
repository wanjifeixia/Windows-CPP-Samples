// MonitorFile.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <windows.h>
using namespace std;

//目录监控线程
UINT MonitorFileThreadProc(LPVOID lpVoid)
{
	wchar_t* pszDirectory = (wchar_t*)lpVoid;

	HANDLE hDirectory = ::CreateFile(pszDirectory, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (INVALID_HANDLE_VALUE == hDirectory)
	{
		//ShowError("CreateFile");
		return 1;
	}

	wchar_t szTemp[MAX_PATH] = { 0 };
	BOOL bRet = FALSE;
	DWORD dwRet = 0;
	DWORD dwBufferSize = 2048;

	BYTE* pBuf = new BYTE[dwBufferSize];
	if (NULL == pBuf)
	{
		//ShowError("new");
		return 2;
	}
	FILE_NOTIFY_INFORMATION* pFileNotifyInfo = (FILE_NOTIFY_INFORMATION*)pBuf;

	do
	{
		::RtlZeroMemory(pFileNotifyInfo, dwBufferSize);

		bRet = ::ReadDirectoryChangesW(hDirectory,
			pFileNotifyInfo,
			dwBufferSize,
			FALSE,
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_CREATION |
			FILE_NOTIFY_CHANGE_FILE_NAME | 			// 修改文件名
			FILE_NOTIFY_CHANGE_ATTRIBUTES,			// 修改文件属性
			&dwRet,
			NULL,
			NULL);
		if (FALSE == bRet)
		{
			//ShowError("ReadDirectoryChangesW");
			break;
		}
		// 判断操作类型并显示
		switch (pFileNotifyInfo->Action)
		{
// 		case FILE_ACTION_ADDED:
// 		case FILE_ACTION_REMOVED:
// 		case FILE_ACTION_MODIFIED:
// 		case FILE_ACTION_RENAMED_OLD_NAME:
// 		case FILE_ACTION_RENAMED_NEW_NAME:
		default:
		{
			wprintf(L"[File Added Action]%s\n", (wchar_t*)&pFileNotifyInfo->FileName);
			break;
		}
		}


	} while (bRet);

	::CloseHandle(hDirectory);
	delete[] pBuf;
	pBuf = NULL;

	return 0;
}

int main()
{
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));

	//目录监控线程
	::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MonitorFileThreadProc, (LPVOID)L"C:\\Users\\xxxx\\Desktop", 0, NULL);

	wprintf(L"monitor...\n");
	getchar();
	return 0;
}