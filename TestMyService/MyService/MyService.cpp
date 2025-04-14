#include <iostream>
#include <Windows.h>
#include <winsvc.h>
#include <string>
#include <locale>
#include <memory>

#pragma comment(lib, "Advapi32.lib")

// 服务配置常量
constexpr wchar_t SERVICE_NAME[] = L"MyService";
constexpr wchar_t SERVICE2_NAME[] = L"MyService2";
constexpr wchar_t SERVICE_DESC[] = L"主服务（依赖MyService2）";
constexpr wchar_t SERVICE2_DESC[] = L"基础依赖服务";
constexpr DWORD MAX_WAIT_TIME = 30000;

// 全局服务控制变量
SERVICE_STATUS g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_hServiceStatus = nullptr;
HANDLE g_hStopEvent = nullptr;

// 服务控制函数声明
void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
void WINAPI Service2Main(DWORD argc, LPWSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD dwCtrl);
void ReportServiceState(DWORD dwState, DWORD dwWait = 0);

// 服务管理函数声明
bool InstallServices();
bool UninstallService(const wchar_t* svcName);
bool StartService(const wchar_t* svcName);
bool StopService(const wchar_t* svcName);
void EnumServices();

// 辅助函数
std::wstring GetExePath();
void ShowLastError(const wchar_t* prefix);

// 服务主逻辑实现
void WINAPI ServiceMain(DWORD argc, LPWSTR* argv) {
	g_hServiceStatus = RegisterServiceCtrlHandlerW(SERVICE_NAME, ServiceCtrlHandler);
	if (!g_hServiceStatus) {
		ShowLastError(L"注册服务控制处理器失败");
		return;
	}

	ReportServiceState(SERVICE_START_PENDING);

	g_hStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!g_hStopEvent) {
		ShowLastError(L"创建停止事件失败");
		ReportServiceState(SERVICE_STOPPED);
		return;
	}

	ReportServiceState(SERVICE_RUNNING);
	WaitForSingleObject(g_hStopEvent, INFINITE);

	CloseHandle(g_hStopEvent);
	ReportServiceState(SERVICE_STOPPED);
}

void WINAPI Service2Main(DWORD argc, LPWSTR* argv) {
	g_hServiceStatus = RegisterServiceCtrlHandlerW(SERVICE2_NAME, ServiceCtrlHandler);
	if (!g_hServiceStatus) {
		ShowLastError(L"注册服务2控制处理器失败");
		return;
	}

	ReportServiceState(SERVICE_START_PENDING);

	g_hStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!g_hStopEvent) {
		ShowLastError(L"创建服务2停止事件失败");
		ReportServiceState(SERVICE_STOPPED);
		return;
	}

	ReportServiceState(SERVICE_RUNNING);
	WaitForSingleObject(g_hStopEvent, INFINITE);

	CloseHandle(g_hStopEvent);
	ReportServiceState(SERVICE_STOPPED);
}

void WINAPI ServiceCtrlHandler(DWORD dwCtrl) {
	switch (dwCtrl) {
	case SERVICE_CONTROL_STOP:
		ReportServiceState(SERVICE_STOP_PENDING);
		SetEvent(g_hStopEvent);
		break;
	default:
		break;
	}
}

void ReportServiceState(DWORD dwState, DWORD dwWait) {
	static DWORD dwCheckPoint = 1;

	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwCurrentState = dwState;
	g_ServiceStatus.dwControlsAccepted = (dwState == SERVICE_RUNNING) ?
		SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN : 0;
	g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
	g_ServiceStatus.dwWaitHint = dwWait;
	g_ServiceStatus.dwCheckPoint =
		((dwState == SERVICE_RUNNING) || (dwState == SERVICE_STOPPED)) ? 0 : dwCheckPoint++;

	SetServiceStatus(g_hServiceStatus, &g_ServiceStatus);
}

// 服务管理功能实现
bool InstallServices() {
	// 安装MyService2
	SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
	if (!hSCM) {
		ShowLastError(L"打开服务管理器失败");
		return false;
	}

	std::wstring binPath = GetExePath() + L" -service2";

	SC_HANDLE hService = CreateServiceW(
		hSCM,
		SERVICE2_NAME,
		SERVICE2_DESC,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		binPath.c_str(),
		nullptr, nullptr, nullptr, nullptr, nullptr
	);

	if (!hService) {
		ShowLastError(L"创建基础服务失败");
		CloseServiceHandle(hSCM);
		return false;
	}

	SERVICE_DESCRIPTION sd2 = { const_cast<wchar_t*>(SERVICE2_DESC) };
	ChangeServiceConfig2W(hService, SERVICE_CONFIG_DESCRIPTION, &sd2);
	CloseServiceHandle(hService);

	// 安装MyService并设置依赖
	binPath = GetExePath() + L" -service";
	hService = CreateServiceW(
		hSCM,
		SERVICE_NAME,
		SERVICE_DESC,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		binPath.c_str(),
		nullptr, nullptr, nullptr, nullptr, nullptr
	);

	if (!hService) {
		ShowLastError(L"创建主服务失败");
		UninstallService(SERVICE2_NAME);
		CloseServiceHandle(hSCM);
		return false;
	}

	// 设置依赖关系
	const wchar_t* dependencies = L"MyService2\0\0";
	if (!ChangeServiceConfigW(
		hService,
		SERVICE_NO_CHANGE,
		SERVICE_NO_CHANGE,
		SERVICE_NO_CHANGE,
		nullptr,
		nullptr,
		nullptr,
		dependencies,
		nullptr,
		nullptr,
		nullptr))
	{
		ShowLastError(L"设置服务依赖失败");
		DeleteService(hService);
		UninstallService(SERVICE2_NAME);
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCM);
		return false;
	}

	SERVICE_DESCRIPTION sd = { const_cast<wchar_t*>(SERVICE_DESC) };
	ChangeServiceConfig2W(hService, SERVICE_CONFIG_DESCRIPTION, &sd);

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	std::wcout << L"服务安装成功" << std::endl;
	return true;
}

bool UninstallService(const wchar_t* svcName) {
	SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hSCM) {
		ShowLastError(L"打开服务管理器失败");
		return false;
	}

	SC_HANDLE hService = OpenServiceW(hSCM, svcName, DELETE);
	if (!hService) {
		ShowLastError(L"打开服务失败");
		CloseServiceHandle(hSCM);
		return false;
	}

	if (!DeleteService(hService)) {
		ShowLastError(L"删除服务失败");
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCM);
		return false;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	std::wcout << L"服务卸载成功: " << svcName << std::endl;
	return true;
}

bool StartService(const wchar_t* svcName) {
	SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hSCM) {
		ShowLastError(L"打开服务管理器失败");
		return false;
	}
	
	SC_HANDLE hService = OpenServiceW(hSCM, svcName, SERVICE_START);
	if (!hService) {
		ShowLastError(L"打开服务失败");
		CloseServiceHandle(hSCM);
		return false;
	}

	if (!::StartServiceW(hService, 0, nullptr)) {
		ShowLastError(L"启动服务失败");
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCM);
		return false;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	std::wcout << L"服务启动命令已发送: " << svcName << std::endl;
	return true;
}

bool StopService(const wchar_t* svcName) {
	SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	if (!hSCM) {
		ShowLastError(L"打开服务管理器失败");
		return false;
	}

	SC_HANDLE hService = OpenServiceW(hSCM, svcName, SERVICE_STOP);
	if (!hService) {
		ShowLastError(L"打开服务失败");
		CloseServiceHandle(hSCM);
		return false;
	}

	SERVICE_STATUS status;
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &status)) {
		ShowLastError(L"停止服务失败");
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCM);
		return false;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	std::wcout << L"服务停止命令已发送: " << svcName << std::endl;
	return true;
}

void EnumServices() {
	SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
	if (!hSCM) {
		ShowLastError(L"打开服务管理器失败");
		return;
	}

	DWORD dwBytesNeeded = 0;
	DWORD dwServices = 0;
	EnumServicesStatusExW(
		hSCM,
		SC_ENUM_PROCESS_INFO,
		SERVICE_WIN32,
		SERVICE_STATE_ALL,
		nullptr,
		0,
		&dwBytesNeeded,
		&dwServices,
		nullptr,
		nullptr
	);

	if (auto buffer = std::make_unique<BYTE[]>(dwBytesNeeded)) {
		auto services = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(buffer.get());
		if (EnumServicesStatusExW(
			hSCM,
			SC_ENUM_PROCESS_INFO,
			SERVICE_WIN32,
			SERVICE_STATE_ALL,
			buffer.get(),
			dwBytesNeeded,
			&dwBytesNeeded,
			&dwServices,
			nullptr,
			nullptr)) {

			for (DWORD i = 0; i < dwServices; ++i) {
				if (wcscmp(services[i].lpServiceName, SERVICE_NAME) == 0 ||
					wcscmp(services[i].lpServiceName, SERVICE2_NAME) == 0)
				{
					std::wcout << L"服务名称: " << services[i].lpServiceName << L"\n"
						<< L"显示名称: " << services[i].lpDisplayName << L"\n"
						<< L"状态: " << (services[i].ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING
							? L"正在运行" : L"已停止")
						<< L"\n------------------------" << std::endl;
				}
			}
		}
	}

	CloseServiceHandle(hSCM);
}

// 辅助函数实现
std::wstring GetExePath() {
	wchar_t szPath[MAX_PATH] = { 0 };
	GetModuleFileNameW(nullptr, szPath, MAX_PATH);
	return std::wstring(L"\"") + szPath + L"\"";
}

void ShowLastError(const wchar_t* prefix) {
	DWORD dwError = GetLastError();
	wchar_t* lpMsgBuf = nullptr;
	FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&lpMsgBuf),
		0,
		nullptr
	);

	std::wcerr << prefix << L" (错误 0x" << std::hex << dwError
		<< L"): " << (lpMsgBuf ? lpMsgBuf : L"未知错误") << std::endl;
	LocalFree(lpMsgBuf);
}

int wmain(int argc, wchar_t* argv[]) {
	std::locale::global(std::locale(""));

	// 服务模式运行
	if (argc > 1) {
		SERVICE_TABLE_ENTRYW DispatchTable[] = {
			{ const_cast<wchar_t*>(SERVICE_NAME), ServiceMain },
			{ const_cast<wchar_t*>(SERVICE2_NAME), Service2Main },
			{ nullptr, nullptr }
		};

		if (wcscmp(argv[1], L"-service") == 0) {
			DispatchTable[0].lpServiceName = const_cast<wchar_t*>(SERVICE_NAME);
			DispatchTable[0].lpServiceProc = ServiceMain;
		}
		else if (wcscmp(argv[1], L"-service2") == 0) {
			DispatchTable[0].lpServiceName = const_cast<wchar_t*>(SERVICE2_NAME);
			DispatchTable[0].lpServiceProc = Service2Main;
		}

		if (!StartServiceCtrlDispatcherW(DispatchTable)) {
			ShowLastError(L"服务启动失败");
			return 1;
		}
		return 0;
	}
	
	// 交互模式
	int iInput = 0;
	bool bFirst = true;
	while (true) {
		if (!bFirst) std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"1. 列举服务\n2. 安装全套服务\n3. 卸载全套服务\n"
			<< L"4. 启动主服务\n5. 停止主服务\n6. 启动基础服务\n7. 停止基础服务\n0. 退出\n请选择操作: ";
		std::wcin >> iInput;

		switch (iInput) {
		case 1:
			EnumServices();
			break;
		case 2:
			InstallServices();
			break;
		case 3:
			UninstallService(SERVICE_NAME);
			UninstallService(SERVICE2_NAME);
			break;
		case 4:
			StartService(SERVICE_NAME);
			break;
		case 5:
			StopService(SERVICE_NAME);
			break;
		case 6:
			StartService(SERVICE2_NAME);
			break;
		case 7:
			StopService(SERVICE2_NAME);
			break;
		case 0:
			return 0;
		default:
			std::wcout << L"无效选项，请重新输入" << std::endl;
		}
	}
}