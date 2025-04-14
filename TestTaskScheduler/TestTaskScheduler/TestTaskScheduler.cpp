#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#include <iostream>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

// 将 HRESULT 转换为可读的错误描述
std::wstring HrToString(HRESULT hr) {
	// 尝试获取系统错误信息
	LPWSTR pMsgBuf = nullptr;
	DWORD nMsgLen = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&pMsgBuf,
		0,
		nullptr
	);

	std::wstring errorMsg;
	if (nMsgLen > 0) {
		errorMsg = pMsgBuf;
		LocalFree(pMsgBuf); // 释放系统分配的内存
	}
	else {
		// 如果系统信息不存在，尝试使用 _com_error
		_com_error err(hr);
		errorMsg = err.ErrorMessage();
	}

	// 清理换行符
	if (!errorMsg.empty() && errorMsg.back() == L'\n') {
		errorMsg.pop_back();
		if (!errorMsg.empty() && errorMsg.back() == L'\r') {
			errorMsg.pop_back();
		}
	}

	return errorMsg;
}

void EnumTask()
{
	//  ------------------------------------------------------
	//  Initialize COM.
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("\nCoInitializeEx failed: %x", hr);
		return;
	}

	//  Set general COM security levels.
	hr = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_PKT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		0,
		NULL);
	if (FAILED(hr))
	{
		printf("\nCoInitializeSecurity failed: %x", hr);
		return;
	}

	//  ------------------------------------------------------
	//  Create an instance of the Task Service. 
	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void**)&pService);
	if (FAILED(hr))
	{
		printf("Failed to CoCreate an instance of the TaskService class: %x", hr);
		CoUninitialize();
		return;
	}

	//  Connect to the local task service.
	hr = pService->Connect(_variant_t(), _variant_t(),
		_variant_t(), _variant_t());
	if (FAILED(hr))
	{
		std::wcerr << L"ITaskService::Connect failed (0x" << std::hex << hr << L"): " << HrToString(hr) << std::endl;
		pService->Release();
		CoUninitialize();
		return;
	}

	//  ------------------------------------------------------
	//  Get the pointer to the root task folder.
	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);

	pService->Release();
	if (FAILED(hr))
	{
		printf("Cannot get Root Folder pointer: %x", hr);
		CoUninitialize();
		return;
	}

	//  -------------------------------------------------------
	//  Get the registered tasks in the folder.
	IRegisteredTaskCollection* pTaskCollection = NULL;
	hr = pRootFolder->GetTasks(NULL, &pTaskCollection);

	pRootFolder->Release();
	if (FAILED(hr))
	{
		printf("Cannot get the registered tasks.: %x", hr);
		CoUninitialize();
		return;
	}

	LONG numTasks = 0;
	hr = pTaskCollection->get_Count(&numTasks);
	if (FAILED(hr))
	{
		printf("Cannot get the task collection.: %x", hr);
		CoUninitialize();
		return;
	}

	if (numTasks == 0)
	{
		printf("\nNo Tasks are currently running");
		pTaskCollection->Release();
		CoUninitialize();
		return;
	}

	printf("\nEnumTask Number of Tasks : %d", numTasks);

	//  -------------------------------------------------------
	//  Visit each task in the folder.
	for (LONG i = 0; i < numTasks; i++)
	{
		IRegisteredTask* pRegisteredTask = NULL;
		_bstr_t taskName;
		TASK_STATE taskState;
		_bstr_t taskStateStr;

		hr = pTaskCollection->get_Item(_variant_t(i + 1), &pRegisteredTask);
		if (FAILED(hr))
		{
			printf("Cannot get the registered task: %x", hr);
			continue;
		}

		hr = pRegisteredTask->get_Name(taskName.GetAddress());
		if (FAILED(hr))
		{
			printf("Cannot get the registered task name: %x", hr);
			pRegisteredTask->Release();
			continue;
		}

		hr = pRegisteredTask->get_State(&taskState);
		if (FAILED(hr))
		{
			printf("Cannot get the registered task state: %x", hr);
		}
		else
		{
			switch (taskState)
			{
			case TASK_STATE_DISABLED:
				taskStateStr = "disabled";
				break;
			case TASK_STATE_QUEUED:
				taskStateStr = "queued";
				break;
			case TASK_STATE_READY:
				taskStateStr = "ready";
				break;
			case TASK_STATE_RUNNING:
				taskStateStr = "running";
				break;
			default:
				taskStateStr = "unknown";
				break;
			}
			printf("\n\tState: %s\tTask Name: %S", (LPCSTR)taskStateStr, (LPCWSTR)taskName);
		}

		pRegisteredTask->Release();
	}

	pTaskCollection->Release();
	CoUninitialize();
	return;
}

void CreateTask(LPCWSTR lpTaskName)
{
	// Initialize COM library
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		std::cerr << "CoInitializeEx failed: " << std::hex << hr << std::endl;
		return;
	}

	// Set COM security levels
	hr = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		0,
		NULL);

	if (FAILED(hr)) {
		std::cerr << "CoInitializeSecurity failed: " << std::hex << hr << std::endl;
		CoUninitialize();
		return;
	}

	// Create the Task Service instance
	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
	if (FAILED(hr)) {
		std::cerr << "CoCreateInstance failed: " << std::hex << hr << std::endl;
		CoUninitialize();
		return;
	}

	// Connect to the task service
	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr)) {
		std::wcerr << L"ITaskService::Connect failed (0x" << std::hex << hr << L"): " << HrToString(hr) << std::endl;

		pService->Release();
		CoUninitialize();
		return;
	}

	// Get the root task folder
	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr)) {
		std::cerr << "Cannot get Root Folder pointer: " << std::hex << hr << std::endl;
		pService->Release();
		CoUninitialize();
		return;
	}

	// Create the task definition
	ITaskDefinition* pTask = NULL;
	hr = pService->NewTask(0, &pTask);
	pService->Release();  // Release the service object
	if (FAILED(hr)) {
		std::cerr << "Failed to create a task definition: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		CoUninitialize();
		return;
	}

	// Define the registration info
	IRegistrationInfo* pRegInfo = NULL;
	hr = pTask->get_RegistrationInfo(&pRegInfo);
	if (FAILED(hr)) {
		std::cerr << "Cannot get registration info pointer: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return;
	}

	pRegInfo->put_Author(_bstr_t(L"Author Name"));
	pRegInfo->Release();  // Release the registration info object

	// Create the principal for the task
	IPrincipal* pPrincipal = NULL;
	hr = pTask->get_Principal(&pPrincipal);
	if (FAILED(hr)) {
		std::cerr << "Cannot get principal pointer: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return;
	}

	pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
	pPrincipal->Release();  // Release the principal object

	// Create the task settings
	ITaskSettings* pSettings = NULL;
	hr = pTask->get_Settings(&pSettings);
	if (FAILED(hr)) {
		std::cerr << "Cannot get settings pointer: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return;
	}

	pSettings->put_StartWhenAvailable(VARIANT_TRUE);
	pSettings->Release();  // Release the settings object

	// Create the trigger for the task
	ITriggerCollection* pTriggerCollection = NULL;
	hr = pTask->get_Triggers(&pTriggerCollection);
	if (FAILED(hr)) {
		std::cerr << "Cannot get trigger collection: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return;
	}

	ITrigger* pTrigger = NULL;
	hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
	pTriggerCollection->Release();  // Release the trigger collection object
	if (FAILED(hr)) {
		std::cerr << "Cannot create the trigger: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return;
	}

	// Define the trigger
	ILogonTrigger* pLogonTrigger = NULL;
	hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
	pTrigger->Release();  // Release the trigger object
	if (FAILED(hr)) {
		std::cerr << "QueryInterface call failed for ILogonTrigger: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return;
	}

	pLogonTrigger->put_Id(_bstr_t(L"Trigger1"));
	pLogonTrigger->Release();  // Release the logon trigger object

	// Create the action for the task
	IActionCollection* pActionCollection = NULL;
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr)) {
		std::cerr << "Cannot get action collection: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return;
	}

	IAction* pAction = NULL;
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	pActionCollection->Release();  // Release the action collection object
	if (FAILED(hr)) {
		std::cerr << "Cannot create the action: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return;
	}

	// Define the action
	IExecAction* pExecAction = NULL;
	hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
	pAction->Release();  // Release the action object
	if (FAILED(hr)) {
		std::cerr << "QueryInterface call failed for IExecAction: " << std::hex << hr << std::endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return;
	}

	pExecAction->put_Path(_bstr_t(L"notepad.exe"));
	pExecAction->Release();  // Release the exec action object

	// Register the task in the root folder
	IRegisteredTask* pRegisteredTask = NULL;
	hr = pRootFolder->RegisterTaskDefinition(
		_bstr_t(lpTaskName),
		pTask,
		TASK_CREATE_OR_UPDATE,
		_variant_t(),
		_variant_t(),
		TASK_LOGON_INTERACTIVE_TOKEN,
		_variant_t(L""),
		&pRegisteredTask);

	if (FAILED(hr)) {
		std::wcerr << L"任务创建失败 (0x" << std::hex << hr << L"): " << HrToString(hr) << std::endl;
	}
	else {
		printf("任务已成功创建:%ws", lpTaskName);
	}

	// Clean up
	if (pRegisteredTask) pRegisteredTask->Release();
	pRootFolder->Release();
	pTask->Release();
	CoUninitialize();
}

int DeleteTask(LPCWSTR lpTaskName)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("CoInitializeEx 失败: 0x%08lX\n", hr);
		return 1;
	}

	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
	if (FAILED(hr))
	{
		printf("创建 TaskService 实例失败: 0x%08lX\n", hr);
		CoUninitialize();
		return 1;
	}

	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr))
	{
		printf("连接任务服务失败: 0x%08lX\n", hr);
		pService->Release();
		CoUninitialize();
		return 1;
	}

	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr))
	{
		printf("获取根文件夹失败: 0x%08lX\n", hr);
		pService->Release();
		CoUninitialize();
		return 1;
	}

	hr = pRootFolder->DeleteTask(_bstr_t(lpTaskName), 0);
	if (SUCCEEDED(hr))
	{
		printf("任务已成功删除:%ws", lpTaskName);
	}
	else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
	{
		printf("错误：任务 MyTask 不存在\n");
	}
	else
	{
		_com_error err(hr);
		printf("删除任务失败: %ls\n", err.ErrorMessage());
	}

	pRootFolder->Release();
	pService->Release();
	CoUninitialize();

	return 0;
}

void CreateTaskFolder(LPCWSTR lpTaskFolder)
{
	// Initialize COM library
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		std::cerr << "CoInitializeEx failed: " << std::hex << hr << std::endl;
		return;
	}

	// Set general COM security levels
	hr = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		0,
		NULL);

	if (FAILED(hr)) {
		std::cerr << "CoInitializeSecurity failed: " << std::hex << hr << std::endl;
		CoUninitialize();
		return;
	}

	// Create the Task Service instance
	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
	if (FAILED(hr)) {
		_com_error err(hr);
		std::cerr << "CoCreateInstance failed: " << std::hex << hr << err.ErrorMessage() << std::endl;
		CoUninitialize();
		return;
	}

	// Connect to the task service
	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr)) {
		_com_error err(hr);
		std::wcerr << L"ITaskService::Connect failed (0x" << std::hex << hr << L"): " << HrToString(hr) << std::endl;
		MessageBoxW(0, err.ErrorMessage(), 0, 0);
		pService->Release();
		CoUninitialize();
		return;
	}

	// Get the root task folder
	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr)) {
		std::cerr << "Cannot get Root Folder pointer: " << std::hex << hr << std::endl;
		pService->Release();
		CoUninitialize();
		return;
	}

	// Create the \GoogleSystem\GoogleUpdater folder
	ITaskFolder* pNewFolder = NULL;
	hr = pRootFolder->CreateFolder(_bstr_t(lpTaskFolder), _variant_t(L""), &pNewFolder);
	if (FAILED(hr)) {
		std::wcerr << L"任务计划文件夹创建失败 (0x" << std::hex << hr << L"): " << HrToString(hr) << std::endl;
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return;
	}
	else {
		std::wcout << L"任务计划文件夹创建成功:" << lpTaskFolder << std::endl;
	}

	// Clean up
	if (pNewFolder) pNewFolder->Release();
	pRootFolder->Release();
	pService->Release();
	CoUninitialize();
}

// 删除任务计划文件夹函数
// 参数：
//   folderPath - 文件夹路径（如 L"\\MyFolder" 或 L"MyFolder"）
//   recursive  - 是否递归删除子文件夹和任务
// 返回值：Windows HRESULT
HRESULT DeleteTaskFolder(LPCWSTR lpTaskFolder, BOOL recursive = TRUE)
{
	// 初始化COM库
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		printf("COM初始化失败: 0x%08lX\n", hr);
		return hr;
	}

	ITaskService* pService = NULL;
	ITaskFolder* pRootFolder = NULL;

	// 创建任务服务实例
	hr = CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void**)&pService);
	if (FAILED(hr)) return hr;

	// 连接到本地任务服务
	hr = pService->Connect(_variant_t(),  // 本地计算机
		_variant_t(),  // 用户名（空表示当前用户）
		_variant_t(),  // 域（空表示无）
		_variant_t()); // 密码
	if (FAILED(hr)) {
		pService->Release();
		return hr;
	}

	// 获取根文件夹
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr)) {
		pService->Release();
		return hr;
	}

	// 执行文件夹删除操作
	LONG flags = recursive ? 0 : 0x2; // 0x2 = 不递归
	hr = pRootFolder->DeleteFolder(_bstr_t(lpTaskFolder), flags);

	if (SUCCEEDED(hr))
		std::wcout << L"任务计划文件夹删除成功:" << lpTaskFolder << std::endl;
	else
		std::wcerr << L"任务计划文件夹删除失败 (0x" << std::hex << hr << L"): " << HrToString(hr) << std::endl;


	// 释放资源
	pRootFolder->Release();
	pService->Release();

	// 清理COM库
	CoUninitialize();
	return hr;
}

int main()
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

		std::wcout << L"输入1 = 枚举任务计划" << std::endl;
		std::wcout << L"输入2 = 创建任务计划" << std::endl;
		std::wcout << L"输入3 = 删除任务计划" << std::endl;
		std::wcout << L"输入4 = 创建任务计划文件夹" << std::endl;
		std::wcout << L"输入5 = 删除任务计划文件夹" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			EnumTask();
		}
		else if (iInput == 2)
		{
			CreateTask(L"MyTask");
		}
		else if (iInput == 3)
		{
			DeleteTask(L"MyTask");
		}
		else if (iInput == 4)
		{
			CreateTaskFolder(L"MyTaskFolder\\MyTask");
		}
		else if (iInput == 5)
		{
			DeleteTaskFolder(L"MyTaskFolder\\MyTask");
			DeleteTaskFolder(L"MyTaskFolder");
		}
	}
	return 0;
}