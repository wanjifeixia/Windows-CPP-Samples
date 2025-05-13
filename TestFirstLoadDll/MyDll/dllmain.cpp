#include <Windows.h>
#include <iostream>

extern "C" __declspec(dllexport) int MyCreateEvent()
{
	// 初始化代码
	std::cout << "[MyDll.dll] MyCreateEvent" << std::endl;

	// 创建一个命名事件
	HANDLE hEvent = CreateEvent(
		nullptr,   // 默认安全属性
		TRUE,      // 手动重置事件
		FALSE,     // 初始状态为非信号状态
		L"Global\\MyNamedEvent_MyCreateEvent"  // 事件名称
	);

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
	{
		std::cout << "[MyDll.dll] DLL_PROCESS_ATTACH" << std::endl;

		// 创建一个命名事件
		HANDLE hEvent = CreateEvent(
			nullptr,   // 默认安全属性
			TRUE,      // 手动重置事件
			FALSE,     // 初始状态为非信号状态
			L"Global\\MyNamedEvent_DLL_PROCESS_ATTACH"  // 事件名称
		);

		break;
	}
	case DLL_THREAD_ATTACH:
		std::cout << "[MyDll.dll] DLL_THREAD_ATTACH" << std::endl;
		break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

