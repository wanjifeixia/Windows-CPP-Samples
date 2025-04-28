#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

std::string g_strIcmpPrefix = "[Prefix_202412061441341864923117730797346000";
BOOL g_bHook = FALSE;

DWORD WINAPI Net_IcmpSendEcho(
	_In_                       HANDLE                   IcmpHandle,
	_In_                       IPAddr                   DestinationAddress,
	_In_reads_bytes_(RequestSize)   LPVOID                   RequestData,
	_In_                       WORD                     RequestSize,
	_In_opt_                   PIP_OPTION_INFORMATION   RequestOptions,
	_Out_writes_bytes_(ReplySize)    LPVOID                   ReplyBuffer,
	_In_range_(>= , sizeof(ICMP_ECHO_REPLY) + RequestSize + 8)
	DWORD                    ReplySize,
	_In_                       DWORD                    Timeout
)
{
	if (!g_bHook)
		return IcmpSendEcho(IcmpHandle, DestinationAddress,
			RequestData, RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);

	char* newRequestData = new char[g_strIcmpPrefix.length() + RequestSize];
	memcpy(newRequestData, g_strIcmpPrefix.c_str(), g_strIcmpPrefix.length());
	memcpy(newRequestData + g_strIcmpPrefix.length(), RequestData, RequestSize);

	// 调用 IcmpSendEcho2Ex 函数
	DWORD dwResult = IcmpSendEcho(IcmpHandle, DestinationAddress,
		newRequestData, g_strIcmpPrefix.length() + RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);

	// 清理分配的内存
	delete[] newRequestData;
	return dwResult;
}

int Ping_IcmpSendEcho(const char* target_ip)
{
	// Declare and initialize variables
	HANDLE hIcmpFile;
	unsigned long ipaddr = INADDR_NONE;
	DWORD dwRetVal = 0;
	char SendData[32] = "IcmpSendEcho Ping test data";
	LPVOID ReplyBuffer = NULL;
	DWORD ReplySize = 0;

	ipaddr = inet_addr(target_ip);
	if (ipaddr == INADDR_NONE) {
		printf("usage: %s IP address\n", target_ip);
		return 1;
	}

	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE) {
		printf("\tUnable to open handle.\n");
		printf("IcmpCreatefile returned error: %ld\n", GetLastError());
		return 1;
	}

	ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
	ReplyBuffer = (VOID*)malloc(ReplySize);
	if (ReplyBuffer == NULL) {
		printf("\tUnable to allocate memory\n");
		IcmpCloseHandle(hIcmpFile);
		return 1;
	}

	dwRetVal = Net_IcmpSendEcho(hIcmpFile, ipaddr, SendData, sizeof(SendData),
		NULL, ReplyBuffer, ReplySize, 1000);
	if (dwRetVal != 0) {
		PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
		struct in_addr ReplyAddr;
		ReplyAddr.S_un.S_addr = pEchoReply->Address;
		if (dwRetVal > 1) {
			printf("\tReceived %ld icmp message responses\n", dwRetVal);
			printf("\tInformation from the first response:\n");
		}
		else {
			printf("\tReceived %ld icmp message response\n", dwRetVal);
			printf("\tInformation from this response:\n");
		}
		printf("\t  Received from %s\n", inet_ntoa(ReplyAddr));
		printf("\t  Status = %ld\n",
			pEchoReply->Status);
		printf("\t  Roundtrip time = %ld milliseconds\n",
			pEchoReply->RoundTripTime);
	}
	else {
		printf("\tCall to IcmpSendEcho failed.\n");
		printf("\tIcmpSendEcho returned error: %ld\n", GetLastError());
	}
	IcmpCloseHandle(hIcmpFile);
	return 0;
}

DWORD WINAPI Net_IcmpSendEcho2(
	_In_                        HANDLE                   IcmpHandle,
	_In_opt_                    HANDLE                   Event,
#ifdef PIO_APC_ROUTINE_DEFINED
	_In_opt_                    PIO_APC_ROUTINE          ApcRoutine,
#else
	_In_opt_                    FARPROC                  ApcRoutine,
#endif
	_In_opt_                    PVOID                    ApcContext,
	_In_                        IPAddr                   DestinationAddress,
	_In_reads_bytes_(RequestSize)    LPVOID                   RequestData,
	_In_                        WORD                     RequestSize,
	_In_opt_                    PIP_OPTION_INFORMATION   RequestOptions,
	_Out_writes_bytes_(ReplySize)     LPVOID                   ReplyBuffer,
	_In_range_(>= , sizeof(ICMP_ECHO_REPLY) + RequestSize + 8)
	DWORD                    ReplySize,
	_In_                        DWORD                    Timeout
)
{
	if (!g_bHook)
		return IcmpSendEcho2(IcmpHandle, Event, ApcRoutine, ApcContext, DestinationAddress,
			RequestData, RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);

	char* newRequestData = new char[g_strIcmpPrefix.length() + RequestSize];
	memcpy(newRequestData, g_strIcmpPrefix.c_str(), g_strIcmpPrefix.length());
	memcpy(newRequestData + g_strIcmpPrefix.length(), RequestData, RequestSize);

	// 调用 IcmpSendEcho2Ex 函数
	DWORD dwResult = IcmpSendEcho2(IcmpHandle, Event, ApcRoutine, ApcContext, DestinationAddress,
		newRequestData, g_strIcmpPrefix.length() + RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);

	// 清理分配的内存
	delete[] newRequestData;
	return dwResult;
}

int Ping_IcmpSendEcho2(const char* target_ip)
{
	// Declare and initialize variables.
	HANDLE hIcmpFile;
	unsigned long ipaddr = INADDR_NONE;
	DWORD dwRetVal = 0;
	DWORD dwError = 0;
	char SendData[] = "IcmpSendEcho2 Ping test data";
	LPVOID ReplyBuffer = NULL;
	DWORD ReplySize = 0;

	ipaddr = inet_addr(target_ip);
	if (ipaddr == INADDR_NONE) {
		printf("usage: %s IP address\n", target_ip);
		return 1;
	}

	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE) {
		printf("\tUnable to open handle.\n");
		printf("IcmpCreatefile returned error: %ld\n", GetLastError());
		return 1;
	}

	// Allocate space for a single reply.
	ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData) + 8;
	ReplyBuffer = (VOID*)malloc(ReplySize);
	if (ReplyBuffer == NULL) {
		printf("\tUnable to allocate memory for reply buffer\n");
		IcmpCloseHandle(hIcmpFile);
		return 1;
	}

	dwRetVal = Net_IcmpSendEcho2(hIcmpFile, NULL, NULL, NULL,
		ipaddr, SendData, sizeof(SendData), NULL,
		ReplyBuffer, ReplySize, 1000);
	if (dwRetVal != 0) {
		PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
		struct in_addr ReplyAddr;
		ReplyAddr.S_un.S_addr = pEchoReply->Address;
		if (dwRetVal > 1) {
			printf("\tReceived %ld icmp message responses\n", dwRetVal);
			printf("\tInformation from the first response:\n");
		}
		else {
			printf("\tReceived %ld icmp message response\n", dwRetVal);
			printf("\tInformation from this response:\n");
		}
		printf("\t  Received from %s\n", inet_ntoa(ReplyAddr));
		printf("\t  Status = %ld  ", pEchoReply->Status);
		switch (pEchoReply->Status) {
		case IP_DEST_HOST_UNREACHABLE:
			printf("(Destination host was unreachable)\n");
			break;
		case IP_DEST_NET_UNREACHABLE:
			printf("(Destination Network was unreachable)\n");
			break;
		case IP_REQ_TIMED_OUT:
			printf("(Request timed out)\n");
			break;
		default:
			printf("\n");
			break;
		}

		printf("\t  Roundtrip time = %ld milliseconds\n",
			pEchoReply->RoundTripTime);
	}
	else {
		printf("Call to IcmpSendEcho2 failed.\n");
		dwError = GetLastError();
		switch (dwError) {
		case IP_BUF_TOO_SMALL:
			printf("\tReplyBufferSize too small\n");
			break;
		case IP_REQ_TIMED_OUT:
			printf("\tRequest timed out\n");
			break;
		default:
			printf("\tExtended error returned: %ld\n", dwError);
			break;
		}
	}
	IcmpCloseHandle(hIcmpFile);
	return 0;
}

DWORD WINAPI Net_IcmpSendEcho2Ex(
		_In_                       HANDLE                   IcmpHandle,
		_In_opt_                   HANDLE                   Event,
#ifdef PIO_APC_ROUTINE_DEFINED
		_In_opt_                   PIO_APC_ROUTINE          ApcRoutine,
#else
		_In_opt_                   FARPROC                  ApcRoutine,
#endif
		_In_opt_                   PVOID                    ApcContext,
		_In_                       IPAddr                   SourceAddress,
		_In_                       IPAddr                   DestinationAddress,
		_In_reads_bytes_(RequestSize)   LPVOID                   RequestData,
		_In_                       WORD                     RequestSize,
		_In_opt_                   PIP_OPTION_INFORMATION   RequestOptions,
		_Out_writes_bytes_(ReplySize)    LPVOID                   ReplyBuffer,
		_In_range_(>= , sizeof(ICMP_ECHO_REPLY) + RequestSize + 8)
		DWORD                    ReplySize,
		_In_                       DWORD                    Timeout
	)
{
	if (!g_bHook)
		return IcmpSendEcho2Ex(IcmpHandle, Event, ApcRoutine, ApcContext, SourceAddress, DestinationAddress,
			RequestData, RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);

	char* newRequestData = new char[g_strIcmpPrefix.length() + RequestSize];
	memcpy(newRequestData, g_strIcmpPrefix.c_str(), g_strIcmpPrefix.length());
	memcpy(newRequestData + g_strIcmpPrefix.length(), RequestData, RequestSize);

	// 调用 IcmpSendEcho2Ex 函数
	DWORD dwResult = IcmpSendEcho2Ex(IcmpHandle, Event, ApcRoutine, ApcContext, SourceAddress, DestinationAddress,
		newRequestData, g_strIcmpPrefix.length() + RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);

	// 清理分配的内存
	delete[] newRequestData;
	return dwResult;
}

void Ping_IcmpSendEcho2Ex(const char* target_ip)
{
	// 创建 ICMP 文件句柄
	HANDLE hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE) {
		std::cerr << "Unable to open ICMP handle. Error: " << GetLastError() << std::endl;
		return;
	}

	// 将 IP 地址转换为网络字节序
	in_addr dest_addr{};
	if (inet_pton(AF_INET, target_ip, &dest_addr) != 1) {
		std::cerr << "Invalid IP address format." << std::endl;
		IcmpCloseHandle(hIcmpFile);
		return;
	}

	// 分配接收缓冲区
	char sendData[] = "IcmpSendEcho2Ex Ping test data";
	DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
	char* replyBuffer = new char[replySize];
	memset(replyBuffer, 0, replySize);

	// 使用 IcmpSendEcho2Ex 发送 ICMP 请求
	DWORD dwRetVal = Net_IcmpSendEcho2Ex(
		hIcmpFile,                       // ICMP 文件句柄
		NULL,                            // 无回调函数
		NULL,                            // 无回调上下文
		NULL,                            // 不使用 Overlapped 结构
		0,
		dest_addr.S_un.S_addr,           // 目标 IP 地址
		sendData,                        // 发送的数据
		sizeof(sendData),                // 数据大小
		NULL,
		replyBuffer,                     // 回复缓冲区
		replySize,                       // 回复缓冲区大小
		1000                              // 超时时间（毫秒）
	);

	if (dwRetVal != 0) {
		// 解析 ICMP 响应
		auto* echoReply = (PICMP_ECHO_REPLY)replyBuffer;
		char replyAddrStr[INET_ADDRSTRLEN]; // IPv4 地址字符串缓冲区
		inet_ntop(AF_INET, &echoReply->Address, replyAddrStr, INET_ADDRSTRLEN);

		std::cout << "Reply from " << replyAddrStr
			<< ": bytes=" << echoReply->DataSize
			<< " time=" << echoReply->RoundTripTime << "ms"
			<< " TTL=" << (int)echoReply->Options.Ttl << std::endl;
	}
	else {
		std::cerr << "Request timed out or failed. Error: " << GetLastError() << std::endl;
	}

	// 释放资源
	delete[] replyBuffer;
	IcmpCloseHandle(hIcmpFile);
}

DWORD WINAPI Net_Icmp6SendEcho2(
	_In_                     HANDLE                   IcmpHandle,
	_In_opt_                 HANDLE                   Event,
#ifdef PIO_APC_ROUTINE_DEFINED
	_In_opt_                 PIO_APC_ROUTINE          ApcRoutine,
#else
	_In_opt_                 FARPROC                  ApcRoutine,
#endif
	_In_opt_                 PVOID                    ApcContext,
	_In_                     struct sockaddr_in6* SourceAddress,
	_In_                     struct sockaddr_in6* DestinationAddress,
	_In_reads_bytes_(RequestSize) LPVOID                   RequestData,
	_In_                     WORD                     RequestSize,
	_In_opt_                 PIP_OPTION_INFORMATION   RequestOptions,
	_Out_writes_bytes_(ReplySize)  LPVOID                   ReplyBuffer,
	_In_range_(>= , sizeof(ICMPV6_ECHO_REPLY) + RequestSize + 8)
	DWORD                    ReplySize,
	_In_                     DWORD                    Timeout
)
{
	if (!g_bHook)
		return Icmp6SendEcho2(IcmpHandle, Event, ApcRoutine, ApcContext, SourceAddress, DestinationAddress,
			RequestData, RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);

	char* newRequestData = new char[g_strIcmpPrefix.length() + RequestSize];
	memcpy(newRequestData, g_strIcmpPrefix.c_str(), g_strIcmpPrefix.length());
	memcpy(newRequestData + g_strIcmpPrefix.length(), RequestData, RequestSize);

	// 调用 IcmpSendEcho2Ex 函数
	DWORD dwResult = Icmp6SendEcho2(IcmpHandle, Event, ApcRoutine, ApcContext, SourceAddress, DestinationAddress,
		newRequestData, g_strIcmpPrefix.length() + RequestSize, RequestOptions, ReplyBuffer, ReplySize, Timeout);

	// 清理分配的内存
	delete[] newRequestData;
	return dwResult;
}

int Ping_Icmp6SendEcho2(const char* target_ip)
{
	// 创建 ICMPv6 句柄
	HANDLE hIcmp = Icmp6CreateFile();
	if (hIcmp == INVALID_HANDLE_VALUE) {
		std::cerr << "Icmp6CreateFile failed. Error: " << GetLastError() << "\n";
		WSACleanup();
		return 1;
	}

	// 源地址
	sockaddr_in6 sourceAddr = {};
	sourceAddr.sin6_family = AF_INET6;
	sourceAddr.sin6_addr = in6addr_any;
	sourceAddr.sin6_flowinfo = 0;
	sourceAddr.sin6_port = 0;

	// 设置目标地址为 IPv6 回环地址 (::1)
	sockaddr_in6 destAddr = {};
	destAddr.sin6_family = AF_INET6;
	inet_pton(AF_INET6, target_ip, &destAddr.sin6_addr);

	// 设置发送和接收缓冲区
	char sendBuffer[32] = "Icmp6SendEcho2 Ping test data";
	char replyBuffer[1024] = { 0 };

	// 发起 ICMPv6 回显请求
	DWORD replySize = sizeof(replyBuffer);
	DWORD timeout = 1000; // 超时时间（毫秒）
	DWORD result = Net_Icmp6SendEcho2(
		hIcmp,               // ICMP 句柄
		NULL,                // 重叠 I/O 的事件句柄 (可为 NULL)
		NULL,                // 回调函数 (可为 NULL)
		NULL,                // 上下文 (可为 NULL)
		&sourceAddr,         // 源地址
		&destAddr,           // 目标地址
		sendBuffer,          // 要发送的数据
		sizeof(sendBuffer),  // 数据大小
		NULL,                // IP 选项 (可为 NULL)
		replyBuffer,         // 存储回复的缓冲区
		replySize,           // 回复缓冲区大小
		timeout              // 超时时间
	);

	// 检查结果
	if (result == 0) {
		std::cerr << "Ping failed. Error: " << GetLastError() << "\n";
	}
	else {
		auto* reply = reinterpret_cast<ICMPV6_ECHO_REPLY*>(replyBuffer);
		char addrStr[INET6_ADDRSTRLEN] = { 0 };
		inet_ntop(AF_INET6, &reply->Address.sin6_addr, addrStr, sizeof(addrStr));

		std::cout << "Reply from " << addrStr
			<< ": Status=" << reply->Status
			<< " Roundtrip time: " << reply->RoundTripTime << "ms\n";
	}

	// 释放资源
	IcmpCloseHandle(hIcmp);
	return 0;
}

int main()
{
	std::cerr << "Ping_IcmpSendEcho\t 8.8.8.8" << std::endl;
	Ping_IcmpSendEcho("8.8.8.8");
	
	std::cerr << "\n\nPing_IcmpSendEcho2\t 8.8.8.8" << std::endl;
	Ping_IcmpSendEcho2("8.8.8.8");

	std::cerr << "\n\nPing_IcmpSendEcho2Ex\t 8.8.8.8" << std::endl;
	Ping_IcmpSendEcho2Ex("8.8.8.8");

	std::cerr << "\n\nPing_Icmp6SendEcho2\t 2008::19" << std::endl;
	Ping_Icmp6SendEcho2("2008::19");

	std::cerr << "\n\nPing End";
	getchar();
	return 0;
}