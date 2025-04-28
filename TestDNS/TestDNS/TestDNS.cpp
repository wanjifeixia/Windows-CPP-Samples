#include <iostream>
#include <string>
#include <winsock2.h> // Windows Sockets API
#include <ws2tcpip.h> // For getaddrinfo and inet_ntop
#include <windns.h>

#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "Ws2_32.lib") // Link against the Ws2_32.lib

void resolveDomain(const std::string& domain) {
	WSADATA wsaData; // Structure to store Windows Socket API data
	int wsaStartupResult = WSAStartup(MAKEWORD(2, 2), &wsaData); // Initialize Winsock
	if (wsaStartupResult != 0) {
		std::cerr << "WSAStartup failed with error: " << wsaStartupResult << std::endl;
		return;
	}

	struct addrinfo hints, * res, * p;
	char ipStr[INET6_ADDRSTRLEN]; // Buffer for storing IP address as string

	// Zero out the hints structure
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // Support both IPv4 and IPv6
	hints.ai_socktype = SOCK_STREAM; // Resolve for a TCP connection

	// Resolve the domain name
	int status = getaddrinfo(domain.c_str(), nullptr, &hints, &res);
	if (status != 0) {
		std::cerr << "getaddrinfo failed: " << gai_strerrorA(status) << std::endl;
		WSACleanup();
		return;
	}

	std::cout << "IP addresses for " << domain << ":\n";

	// Loop through the results
	for (p = res; p != nullptr; p = p->ai_next) {
		void* addr;
		std::string ipVersion;

		// Get the pointer to the address itself
		if (p->ai_family == AF_INET) { // IPv4
			addr = &((struct sockaddr_in*)p->ai_addr)->sin_addr;
			ipVersion = "IPv4";
		}
		else if (p->ai_family == AF_INET6) { // IPv6
			addr = &((struct sockaddr_in6*)p->ai_addr)->sin6_addr;
			ipVersion = "IPv6";
		}
		else {
			continue;
		}

		// Convert the IP address to a string
		inet_ntop(p->ai_family, addr, ipStr, sizeof(ipStr));
		std::cout << "  " << ipVersion << ": " << ipStr << std::endl;
	}

	// Free the linked list and clean up Winsock
	freeaddrinfo(res);
	WSACleanup();
}

void resolveDomain_DnsQuery(const std::wstring & domain)
{
	PDNS_RECORD pDnsRecord;
	DNS_STATUS status = DnsQuery_W(
		domain.c_str(),   // 查询的域名
		DNS_TYPE_A,       // 查询 A 记录（IPv4 地址）
		0,                // 无特殊选项
		NULL,             // 无额外参数
		&pDnsRecord,      // 输出的结果记录
		NULL              // 无预留参数
	);

	if (status == 0) { // 查询成功
		PDNS_RECORD pRecord = pDnsRecord;
		while (pRecord) {
			if (pRecord->wType == DNS_TYPE_A)
			{
				DWORD ip = pRecord->Data.A.IpAddress;
				std::string ipStr = std::to_string((ip & 0xFF)) + "." +
					std::to_string((ip >> 8) & 0xFF) + "." +
					std::to_string((ip >> 16) & 0xFF) + "." +
					std::to_string((ip >> 24) & 0xFF);
				std::cout << "IPv4 Address: " << ipStr << std::endl;
			}
			pRecord = pRecord->pNext;
		}
		DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
	}
	else {
		std::wcerr << L"DNS Query failed with error: " << status << L"\n";
	}
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

		std::wcout << L"输入1 = 使用getaddrinfo解析DNS" << std::endl;
		std::wcout << L"输入2 = 使用DnsQuery解析DNS" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			std::string domain;
			std::cout << "Enter domain to resolve: ";
			std::cin >> domain;

			resolveDomain(domain);
		}
		else if (iInput == 2)
		{
			std::wstring domain;
			std::cout << "Enter domain to resolve: ";
			std::wcin >> domain;

			resolveDomain_DnsQuery(domain);
		}
	}
	return 0;
}
