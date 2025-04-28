#include <windows.h>
#include <iostream>
#include <winternl.h>
#include <ntstatus.h>

#pragma comment(lib, "ntdll.lib")

int MyCreateEvent() {
	// 创建一个命名事件对象，初始状态为未触发
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, L"Local\\IpcTest_NamedEvent");
	//HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, L"Session\\1\\IpcTest_NamedEvent");
	if (hEvent == NULL) {
		std::cerr << "CreateEvent failed: " << GetLastError() << std::endl;
		return 1;
	}

	std::cout << "PID:" << GetCurrentProcessId() << std::endl;
	std::cout << "创建一个命名事件对象，初始状态为未触发 CreateEvent(NULL, TRUE, FALSE, L\"MyNamedEvent\");" << std::endl;

	// 模拟一些工作
	Sleep(500000);

	std::cout << "触发命名事件 SetEvent(hEvent);" << std::endl;

	// 触发事件
	if (!SetEvent(hEvent)) {
		std::cerr << "SetEvent failed: " << GetLastError() << std::endl;
		CloseHandle(hEvent);
		return 1;
	}

	std::cout << "命名事件已触发，等待两秒之后退出" << std::endl;

	// 等待一段时间，确保另一个进程可以捕获事件
	Sleep(2000);

	// 关闭事件句柄
	CloseHandle(hEvent);

	std::cout << "进程退出" << std::endl;
	return 0;
}

int MyOpenEvent() {
	// 打开已经存在的命名事件
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Session\\1\\IpcTest_NamedEvent");
	//HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\IpcTest_NamedEvent");
	if (hEvent == NULL) {
		std::cerr << "OpenEvent failed: " << GetLastError() << std::endl;
		return 1;
	}

	std::cout << "PID:" << GetCurrentProcessId() << std::endl;
	std::cout << "打开已经存在的命名事件，等待事件被触发 WaitForSingleObject(hEvent, INFINITE);" << std::endl;

	// 等待事件被触发
	DWORD dwWaitResult = WaitForSingleObject(hEvent, INFINITE);
	if (dwWaitResult == WAIT_OBJECT_0) {
		std::cout << "命名事件已经触发" << std::endl;
	}
	else {
		std::cerr << "WaitForSingleObject failed: " << GetLastError() << std::endl;
	}

	// 关闭事件句柄
	CloseHandle(hEvent);

	std::cout << "进程退出" << std::endl;
	return 0;
}

int MyCreateSection() {
	std::cout << "PID:" << GetCurrentProcessId() << std::endl;
	// 创建一个内存映射文件
	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // 使用物理内存
		NULL,                     // 默认安全属性
		PAGE_READWRITE,           // 可读可写
		0,                        // 文件映射对象的最大大小（高32位）
		4096,                     // 文件映射对象的最大大小（低32位）
		L"Local\\IpcTest_Section"); // 内存映射文件的名字

	if (hMapFile == NULL) {
		std::cerr << "Could not create file mapping object: " << GetLastError() << std::endl;
		return 1;
	}

	// 映射内存映射文件的视图
	void* pBuf = MapViewOfFile(
		hMapFile,                 // 内存映射文件的句柄
		FILE_MAP_ALL_ACCESS,      // 可读可写
		0,                        // 偏移量（高32位）
		0,                        // 偏移量（低32位）
		4096);                   // 映射的字节数

	if (pBuf == NULL) {
		std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
		CloseHandle(hMapFile);
		return 1;
	}

	// 写入数据到共享内存
	const char* message = "Hello from Writer!";
	CopyMemory(pBuf, message, strlen(message) + 1);

	std::cout << "Data written to shared memory: " << (char*)pBuf << std::endl;

	// 等待用户输入，以便Reader进程可以读取数据
	std::cout << "Press Enter to exit..." << std::endl;
	std::cin.get();
	std::cin.get();

	// 清理
	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);

	return 0;
}

int MyOpenSection() {
	std::cout << "PID:" << GetCurrentProcessId() << std::endl;
	// 打开内存映射文件
	HANDLE hMapFile = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,      // 可读可写
		FALSE,                    // 不继承句柄
		L"Session\\1\\IpcTest_Section"); // 内存映射文件的名字

	if (hMapFile == NULL) {
		std::cerr << "Could not open file mapping object: " << GetLastError() << std::endl;
		return 1;
	}

	// 映射内存映射文件的视图
	void* pBuf = MapViewOfFile(
		hMapFile,                 // 内存映射文件的句柄
		FILE_MAP_ALL_ACCESS,      // 可读可写
		0,                        // 偏移量（高32位）
		0,                        // 偏移量（低32位）
		4096);                   // 映射的字节数

	if (pBuf == NULL) {
		std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
		CloseHandle(hMapFile);
		return 1;
	}

	// 读取共享内存中的数据
	std::cout << "Data read from shared memory: " << (char*)pBuf << std::endl;

	// 清理
	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);

	return 0;
}

// 定义事件类型
typedef enum _EVENT_TYPE {
	NotificationEvent,
	SynchronizationEvent
} EVENT_TYPE;

// 定义Native API函数类型
typedef NTSTATUS(NTAPI* _NtCreateEvent)(
	PHANDLE            EventHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	EVENT_TYPE         EventType,
	BOOLEAN            InitialState
	);

typedef NTSTATUS(NTAPI* _NtOpenEvent)(
	PHANDLE            EventHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes
	);

int MyNtCreateEvent(LPCWSTR pEventName) {
	HANDLE hEvent = nullptr;
	NTSTATUS status;
	UNICODE_STRING eventName;
	OBJECT_ATTRIBUTES objAttr;

	// 全局事件名称（跨进程可见）
	RtlInitUnicodeString(&eventName, pEventName);

	InitializeObjectAttributes(
		&objAttr,
		&eventName,
		OBJ_CASE_INSENSITIVE,  // 永久对象
		nullptr,
		nullptr
	);

	// 获取NtCreateEvent函数
	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	auto NtCreateEvent = (_NtCreateEvent)GetProcAddress(ntdll, "NtCreateEvent");

	if (!NtCreateEvent) {
		std::cerr << "[创建者] 错误：无法获取NtCreateEvent" << std::endl;
		return 1;
	}

	// 创建同步事件（初始无信号）
	status = NtCreateEvent(
		&hEvent,
		EVENT_ALL_ACCESS,
		&objAttr,
		SynchronizationEvent,
		FALSE
	);

	if (status == STATUS_SUCCESS) {
		std::wcout << pEventName << std::endl;
		std::cout << "[创建者] 事件创建成功！句柄: 0x"
			<< std::hex << hEvent << "\n"
			<< "请保持本进程运行，然后启动打开者进程..."
			<< std::endl;

		// 阻塞等待用户输入，保持事件存在
		std::cin.get();
		std::cin.get();
		CloseHandle(hEvent);
	}
	else {
		std::cerr << "[创建者] 创建失败 (0x" << std::hex << status << ")"
			<< std::endl;
		return 1;
	}
	return 0;
}

int MyNtOpenEvent(LPCWSTR pEventName) {
	HANDLE hEvent = nullptr;
	NTSTATUS status;
	UNICODE_STRING eventName;
	OBJECT_ATTRIBUTES objAttr;

	RtlInitUnicodeString(&eventName, pEventName);

	InitializeObjectAttributes(
		&objAttr,
		&eventName,
		OBJ_CASE_INSENSITIVE,
		nullptr,
		nullptr
	);

	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	auto NtOpenEvent = (_NtOpenEvent)GetProcAddress(ntdll, "NtOpenEvent");

	if (!NtOpenEvent) {
		std::cerr << "[打开者] 错误：无法获取NtOpenEvent" << std::endl;
		return 1;
	}

	// 尝试打开已存在的事件
	status = NtOpenEvent(
		&hEvent,
		EVENT_MODIFY_STATE,  // 最小必要权限
		&objAttr
	);

	if (status == STATUS_SUCCESS) {
		std::wcout << pEventName << std::endl;
		std::cout << "[打开者] 成功打开事件！句柄: 0x"
			<< std::hex << hEvent
			<< std::endl;

		// 示例：触发事件（可选）
		// SetEvent(hEvent);
		CloseHandle(hEvent);
	}
	else {
		std::cerr << "[打开者] 打开失败 (0x" << std::hex << status << ")\n"
			<< "提示：请先运行创建者进程！"
			<< std::endl;
		return 1;
	}
	return 0;
}

int main()
{
	// 设置本地化
	std::locale::global(std::locale(""));

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = 创建一个Event" << std::endl;
		std::wcout << L"输入2 = 打开已经存在的Event" << std::endl;
		std::wcout << L"输入3 = 创建一个Section" << std::endl;
		std::wcout << L"输入4 = 打开已经存在的Section" << std::endl;
		std::wcout << L"输入5 = NtCreateEvent创建一个Event" << std::endl;
		std::wcout << L"输入6 = NtOpenEvent打开已经存在的Event" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			MyCreateEvent();
		}
		else if (iInput == 2)
		{
			MyOpenEvent();
		}
		else if (iInput == 3)
		{
			MyCreateSection();
		}
		else if (iInput == 4)
		{
			MyOpenSection();
		}
		else if (iInput == 5)
		{
			wchar_t eventNameStr[] = L"\\Sessions\\1\\BaseNamedObjects\\Local\\IpcTest_NamedEvent";
			MyNtCreateEvent(eventNameStr);
		}
		else if (iInput == 6)
		{
			wchar_t eventNameStr[] = L"\\Sessions\\1\\BaseNamedObjects\\Session\\1\\IpcTest_NamedEvent";
			MyNtOpenEvent(eventNameStr);
		}
	}
}