#include <curl/curl.h>
#include <iostream>
#include <vector>
#include <winternl.h>
#include <iostream>
#pragma comment(lib, "libcurl.lib")

// 回调函数，用于处理接收到的数据
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
	size_t totalSize = size * nmemb;
	output->append((char*)contents, totalSize);
	return totalSize;
}

int main()
{
	// 初始化 libcurl
	CURL* curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	// 创建一个新的 CURL 句柄
	curl = curl_easy_init();
	if (curl) {
		// 设置请求的 URL
		curl_easy_setopt(curl, CURLOPT_URL, "http://www.baidu.com");

		// 设置数据接收回调函数
		std::string response;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

		// 执行 HTTP 请求
		res = curl_easy_perform(curl);

		// 检查执行结果
		if (res != CURLE_OK)
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		else
			std::cout << "Response: " << response << std::endl;

		// 清理资源
		curl_easy_cleanup(curl);
	}

	// 全局清理
	curl_global_cleanup();

	getchar();
	return 0;
}