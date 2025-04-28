#include <windows.h>
#include <iostream>

#define PIPE_NAME TEXT("\\\\.\\pipe\\MyTestNamedPipe")

int MyServer() {
	HANDLE hNamedPipe;
	BOOL bClientConnected;

	// 创建命名管道
	hNamedPipe = CreateNamedPipe(
		PIPE_NAME,              // 管道名称
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,     // 读/写访问
		PIPE_WAIT |     // 消息类型管道
		PIPE_TYPE_BYTE | // 消息读取模式
		PIPE_READMODE_BYTE | // 消息读取模式
		PIPE_ACCEPT_REMOTE_CLIENTS,              // 阻塞模式
		PIPE_UNLIMITED_INSTANCES, // 实例数目
		0,						// 输出缓冲区大小
		0,                    // 输入缓冲区大小
		3000,                 // 默认超时
		NULL                  // 默认安全属性
	);

	if (hNamedPipe == INVALID_HANDLE_VALUE) {
		std::cout << "创建命名管道失败，错误代码: " << GetLastError() << std::endl;
		return 1;
	}

	std::cout << "等待客户端连接..." << std::endl;

	// 等待客户端连接到管道
	bClientConnected = ConnectNamedPipe(hNamedPipe, NULL) ?
		TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

	if (bClientConnected) {
		std::cout << "客户端已连接。" << std::endl;

		// 从客户端读取数据
		char buffer[512];
		DWORD bytesRead;
		BOOL success = ReadFile(
			hNamedPipe,              // 管道句柄
			buffer,                  // 接收缓冲区
			sizeof(buffer),          // 缓冲区大小
			&bytesRead,              // 实际读取的字节数
			NULL                     // 无重叠结构
		);

		if (success) {
			std::cout << "从客户端收到数据: " << buffer << std::endl;
		}
		else {
			std::cout << "读取客户端消息失败，错误代码: " << GetLastError() << std::endl;
		}

		// 向客户端发送响应消息
		const char* response = "Hello from server!";
		DWORD bytesWritten;
		success = WriteFile(
			hNamedPipe,              // 管道句柄
			response,                // 要发送的数据
			strlen(response) + 1,    // 数据长度（包括 '\0' 终止符）
			&bytesWritten,           // 实际写入的字节数
			NULL                     // 无重叠结构
		);

		if (success) {
			std::cout << "成功发送响应消息给客户端。" << std::endl;
			Sleep(-1);
		}
		else {
			std::cout << "发送消息失败，错误代码: " << GetLastError() << std::endl;
		}
	}
	else {
		std::cout << "客户端连接失败，错误代码: " << GetLastError() << std::endl;
	}

	// 关闭管道句柄
	CloseHandle(hNamedPipe);

	return 0;
}

int MyClient() {
	HANDLE hPipe;
	BOOL success;
	char buffer[512];
	DWORD bytesRead, bytesWritten;

	// 连接到命名管道
	hPipe = CreateFile(
		PIPE_NAME,           // 管道名称
		GENERIC_READ |       // 读取访问权限
		GENERIC_WRITE,       // 写入访问权限
		0,                   // 无共享
		NULL,                // 默认安全属性
		OPEN_EXISTING,       // 打开现有管道
		0,                   // 默认属性
		NULL                 // 无模板文件
	);

	if (hPipe == INVALID_HANDLE_VALUE) {
		std::cout << "连接到管道失败，错误代码: " << GetLastError() << std::endl;
		return 1;
	}

	std::cout << "成功连接到服务器管道。" << std::endl;
	
	// 向服务器发送消息
	const char* message = "Hello from client!";
	success = WriteFile(
		hPipe,                // 管道句柄
		message,              // 要发送的数据
		strlen(message) + 1,  // 数据长度（包括 '\0' 终止符）
		&bytesWritten,        // 实际写入的字节数
		NULL                  // 无重叠结构
	);

	if (success) {
		std::cout << "成功发送消息到服务器。" << std::endl;
	}
	else {
		std::cout << "发送消息失败，错误代码: " << GetLastError() << std::endl;
	}

	// 从服务器读取响应
	success = ReadFile(
		hPipe,                // 管道句柄
		buffer,               // 接收缓冲区
		sizeof(buffer),       // 缓冲区大小
		&bytesRead,           // 实际读取的字节数
		NULL                  // 无重叠结构
	);

	if (success) {
		std::cout << "收到服务器消息: " << buffer << std::endl;
		Sleep(-1);
	}
	else {
		std::cout << "读取服务器消息失败，错误代码: " << GetLastError() << std::endl;
	}

	// 关闭管道句柄
	CloseHandle(hPipe);

	return 0;
}


int main() {
	int iType = 0;
	while (true)
	{
		std::cout << "请选择命名管道（1:服务器 2:客户端）:";
		std::cin >> iType;
		if (iType == 1 || iType == 2)
			break;
	}

	auto result = (iType == 1 ? MyServer() : MyClient());
	std::cin >> result;
	return result;
}