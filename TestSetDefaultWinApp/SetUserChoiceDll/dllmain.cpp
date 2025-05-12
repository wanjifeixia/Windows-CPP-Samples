#include <stdio.h>
#include <ShlObj_core.h>
#include "../TestSetDefaultWinApp/WindowsUserChoice.h"
#include "detours.h"

#if defined _M_X64
#pragma comment(lib, "detours.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "detours.x86.lib")
#endif

#ifdef SETUSERCHOICEDLL_EXPORTS
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif

// 导出函数：设置文件关联
extern "C" API BOOL __cdecl SetUserChoice(LPCWSTR lpszExt, LPCWSTR lpszTargetApp);

// 共享数据结构（严格1字节对齐）
#pragma pack(push, 1)
struct SharedData {
	WCHAR szExt[32];          // 文件扩展名（如 L".bmp"）
	WCHAR szTargetApp[MAX_PATH]; // 目标程序路径
	DWORD dwReadyFlag;         // 数据就绪标志（1=就绪）
	ULONG_PTR hEvent;
};
#pragma pack(pop)

#define SHARED_MEM_NAME   L"Global\\SetUserChoiceDll_Mem"

// 全局共享资源句柄
HANDLE g_hMapFile = NULL;
SharedData* g_pSharedData = NULL;
HANDLE g_hEvent = NULL;

// 共享内存管理模块
BOOL InitSharedMemory(LPCWSTR lpszExt, LPCWSTR lpszTargetApp)
{
	// 创建或打开内存映射文件
	g_hMapFile = CreateFileMappingW(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(SharedData),
		SHARED_MEM_NAME
	);
	if (!g_hMapFile) return FALSE;

	// 映射视图
	g_pSharedData = (SharedData*)MapViewOfFile(g_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
	if (!g_pSharedData) {
		CloseHandle(g_hMapFile);
		return FALSE;
	}

	// 写入数据
	ZeroMemory(g_pSharedData, sizeof(SharedData));
	wcsncpy_s(g_pSharedData->szExt, lpszExt, _TRUNCATE);
	wcsncpy_s(g_pSharedData->szTargetApp, lpszTargetApp, _TRUNCATE);
	g_pSharedData->dwReadyFlag = 1;

	// 创建同步事件
	g_hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
	return g_hEvent != NULL;
}

void ClearSharedMemory()
{
	if (g_pSharedData) UnmapViewOfFile(g_pSharedData);
	if (g_hMapFile) CloseHandle(g_hMapFile);
	if (g_hEvent) CloseHandle(g_hEvent);
}

// 导出函数实现
BOOL __cdecl SetUserChoice(LPCWSTR lpszExt, LPCWSTR lpszTargetApp)
{
	// 初始化共享内存
	if (!InitSharedMemory(lpszExt, lpszTargetApp))
	{
		printf("[!] 共享内存初始化失败 (Error %d)\n", GetLastError());
		return FALSE;
	}

	// 获取当前DLL路径
	HMODULE hModule = NULL;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&SetUserChoice, &hModule))
	{
		printf("[!] GetModuleHandleExW 失败。错误码: %d\n", GetLastError());
		return FALSE;
	}
	CHAR szDllPath[MAX_PATH] = { 0 };
	if (!GetModuleFileNameA(hModule, szDllPath, MAX_PATH))
	{
		printf("[!] GetModuleFileNameA 失败。错误码: %d\n", GetLastError());
		return FALSE;
	}

	// 使用DetourCreateProcessWithDllExW创建挂起进程
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };
	BOOL success = DetourCreateProcessWithDllExW(
		L"C:\\Windows\\System32\\OpenWith.exe", // 目标进程路径
		NULL,                                    // 命令行
		NULL,                                    // 进程属性
		NULL,                                    // 线程属性
		FALSE,                                   // 不继承句柄
		CREATE_SUSPENDED,                       // 创建标志：挂起主线程
		NULL,                                    // 环境变量
		NULL,                                    // 当前目录
		&si,                                     // 启动信息
		&pi,                                     // 进程信息
		szDllPath,                               // 注入的DLL路径（ANSI）
		NULL                                     // 默认CreateProcess实现
	);

	if (!success)
	{
		printf("[!] 注入失败 (错误码: %d)\n", GetLastError());
		return FALSE;
	}

	// 复制事件句柄到目标进程
	HANDLE hEventInTarget = NULL;
	if (DuplicateHandle(
		GetCurrentProcess(),   // 源进程（当前进程）
		g_hEvent,              // 源句柄
		pi.hProcess,           // 目标进程句柄
		&hEventInTarget,       // 目标句柄
		EVENT_MODIFY_STATE,    // 访问权限（触发事件）
		FALSE,                 // 不可继承
		0                      // 无特殊选项
	))
	{
		// 将目标进程的句柄存入共享内存
		g_pSharedData->hEvent = reinterpret_cast<ULONG_PTR>(hEventInTarget);

		// 恢复线程并等待操作完成
		ResumeThread(pi.hThread);
		WaitForSingleObject(g_hEvent, 6 * 1000);
	}
	else
	{
		printf("[!] DuplicateHandle 失败 (Error %d)\n", GetLastError());
	}

	// 清理进程句柄
	TerminateProcess(pi.hProcess, 0);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	ClearSharedMemory();
	return TRUE;
}

static bool SetUserChoiceRegistry(const wchar_t* aExt, const wchar_t* aProgID, const bool aRegRename, const wchar_t* aHash);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason, LPVOID lpReserved)
{
	if (ul_reason == DLL_PROCESS_ATTACH)
	{
		// 检测是否在openwith.exe中运行
		WCHAR szExeName[MAX_PATH];
		GetModuleFileNameW(NULL, szExeName, MAX_PATH);
		if (wcsstr(szExeName, L"OpenWith.exe") || wcsstr(szExeName, L"openwith.exe"))
		{
			// 打开共享内存
			//MessageBoxW(0, L"OpenFileMappingW", 0, 0);
			g_hMapFile = OpenFileMappingW(FILE_MAP_READ, FALSE, SHARED_MEM_NAME);
			if (!g_hMapFile)
			{
				return TRUE;
			}

			g_pSharedData = (SharedData*)MapViewOfFile(g_hMapFile, FILE_MAP_READ, 0, 0, sizeof(SharedData));
			if (g_pSharedData && g_pSharedData->dwReadyFlag == 1)
			{
				static std::unique_ptr<wchar_t[]> sid = UserChoice::GetCurrentUserStringSid();
				SYSTEMTIME systemTime = {};
				GetSystemTime(&systemTime);
				auto hash = UserChoice::GenerateUserChoiceHash(g_pSharedData->szExt, sid.get(), g_pSharedData->szTargetApp, systemTime);
				SetUserChoiceRegistry(g_pSharedData->szExt, g_pSharedData->szTargetApp, false, hash.get());

				// 通知操作完成
				HANDLE hEvent = reinterpret_cast<HANDLE>(g_pSharedData->hEvent);
				if (hEvent) {
					SetEvent(hEvent);   // 触发事件
					CloseHandle(hEvent); // 关闭目标进程中的句柄
				}
			}
			UnmapViewOfFile(g_pSharedData);
			CloseHandle(g_hMapFile);
		}
	}
	return TRUE;
}

static bool SetUserChoiceRegistry(const wchar_t* aExt, const wchar_t* aProgID, const bool aRegRename, const wchar_t* aHash)
{
	auto assocKeyPath = UserChoice::GetAssociationKeyPath(aExt);
	if (!assocKeyPath) {
		return false;
	}

	LSTATUS ls;
	HKEY rawAssocKey;
	ls = ::RegOpenKeyExW(HKEY_CURRENT_USER, assocKeyPath.get(), 0,
		KEY_READ | KEY_WRITE, &rawAssocKey);
	if (ls != ERROR_SUCCESS) {
		return false;
	}

	if (aRegRename) {
		// Registry keys in the HKCU should be writable by applications run by the
		// hive owning user and should not be lockable -
		// https://web.archive.org/web/20230308044345/https://devblogs.microsoft.com/oldnewthing/20090326-00/?p=18703.
		// Unfortunately some kernel drivers lock a set of protocol and file
		// association keys. Renaming a non-locked ancestor is sufficient to fix
		// this.
		ls = ::RegRenameKey(rawAssocKey, nullptr, L"EndeskTempKey");
		if (ls != ERROR_SUCCESS) {
			return false;
		}
	}

	auto subkeysUpdated = [&] {
		// Windows file association keys are read-only (Deny Set Value) for the
		// User, meaning they can not be modified but can be deleted and recreated.
		// We don't set any similar special permissions. Note: this only applies to
		// file associations, not URL protocols.
		if (aExt && aExt[0] == '.') {
			ls = ::RegDeleteKeyW(rawAssocKey, L"UserChoice");
			if (ls != ERROR_SUCCESS) {
				return false;
			}
		}

		HKEY rawUserChoiceKey;
		ls = ::RegCreateKeyExW(rawAssocKey, L"UserChoice", 0, nullptr,
			0 /* options */, KEY_READ | KEY_WRITE,
			0 /* security attributes */, &rawUserChoiceKey,
			nullptr);
		if (ls != ERROR_SUCCESS) {
			return false;
		}

		DWORD progIdByteCount = (::lstrlenW(aProgID) + 1) * sizeof(wchar_t);
		ls = ::RegSetValueExW(rawUserChoiceKey, L"ProgID", 0, REG_SZ,
			reinterpret_cast<const unsigned char*>(aProgID),
			progIdByteCount);
		if (ls != ERROR_SUCCESS) {
			return false;
		}

		DWORD hashByteCount = (::lstrlenW(aHash) + 1) * sizeof(wchar_t);
		ls = ::RegSetValueExW(rawUserChoiceKey, L"Hash", 0, REG_SZ, reinterpret_cast<const unsigned char*>(aHash), hashByteCount);
		if (ls != ERROR_SUCCESS) {
			return false;
		}

		return true;
	}();

	if (aRegRename) {
		// Rename back regardless of whether we successfully modified the subkeys to
		// minimally attempt to return the registry the way we found it. If this
		// fails the afformetioned kernel drivers have likely changed and there's
		// little we can do to anticipate what proper recovery should look like.
		ls = ::RegRenameKey(rawAssocKey, nullptr, aExt);
		if (ls != ERROR_SUCCESS) {
			return false;
		}
	}

	return subkeysUpdated;
}