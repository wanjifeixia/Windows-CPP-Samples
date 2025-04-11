#include <windows.h>
#include <iostream>
#include <shlobj.h>
#include <oleidl.h>
#include <comdef.h>
#include <vector>
#include <string>
#include <fstream>
#include <shlobj.h>
#include <atlcomcli.h>

using namespace std;

int ClearClip() {
	// 打开剪贴板
	if (!OpenClipboard(NULL)) {
		std::cout << "无法打开剪贴板" << std::endl;
		return 1;
	}

	// 清空剪贴板内容
	if (!EmptyClipboard()) {
		std::cout << "无法清空剪贴板内容" << std::endl;
		CloseClipboard();
		return 1;
	}

	// 关闭剪贴板
	CloseClipboard();
	return 0;
}

// 将 Unicode 字符串设置到剪贴板
bool SetClipboardText(const std::wstring& text) {
	// 打开剪贴板（需要管理员权限或应用程序焦点）
	if (!OpenClipboard(nullptr)) {
		return false; // 打开失败（可能被其他程序占用）
	}

	// 清空剪贴板原有内容
	EmptyClipboard();

	// 计算所需内存大小（包含终止符 \0）
	const size_t bufferSize = (text.length() + 1) * sizeof(wchar_t);

	// 分配全局内存（使用可移动内存块）
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bufferSize);
	if (!hMem) {
		CloseClipboard();
		return false;
	}

	// 锁定内存并获取指针
	wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
	if (!pMem) {
		GlobalFree(hMem);
		CloseClipboard();
		return false;
	}

	// 复制字符串到内存（安全版本防止溢出）
	wcscpy_s(pMem, text.length() + 1, text.c_str());

	// 解锁内存
	GlobalUnlock(hMem);

	// 设置剪贴板数据（使用 Unicode 格式）
	SetClipboardData(CF_UNICODETEXT, hMem);

	// 关闭剪贴板（系统接管内存所有权）
	CloseClipboard();

	std::wcout << L"将文字设置到剪切板成功：" << text << std::endl;
	return true;
}

int GetTextFromClip()
{
	// 打开剪贴板
	if (OpenClipboard(nullptr)) {
		// 获取剪贴板中的数据句柄
		HANDLE clipboardDataHandle = GetClipboardData(CF_UNICODETEXT);

		if (clipboardDataHandle != nullptr) {
			// 锁定句柄以获取指向数据的指针
			wchar_t* clipboardData = static_cast<wchar_t*>(GlobalLock(clipboardDataHandle));

			if (clipboardData != nullptr) {
				// 输出剪贴板中的文本数据
				std::wcout << L"获取当前剪切板中的文字:" << clipboardData << std::endl;

				// 解锁内存
				GlobalUnlock(clipboardDataHandle);
			}
			else {
				std::cerr << "Failed to lock clipboard data handle." << std::endl;
			}
		}
		else {
			std::cerr << "Failed to get clipboard data." << std::endl;
		}

		// 关闭剪贴板
		CloseClipboard();
	}
	else {
		std::cerr << "Failed to open clipboard." << std::endl;
	}
	return 0;
}

LRESULT CALLBACK ClipWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bListening = FALSE;
	switch (uMsg)
	{
	case WM_CREATE:
		bListening = AddClipboardFormatListener(hWnd);
		printf("监听剪切板 - AddClipboardFormatListener:%08X\n", hWnd);
		return bListening ? 0 : -1;

	case WM_DESTROY:
		if (bListening)
		{
			RemoveClipboardFormatListener(hWnd);
			printf("取消监听剪切板 - RemoveClipboardFormatListener\n");
			bListening = FALSE;
		}
		return 0;

	case WM_CLIPBOARDUPDATE:
		printf("监听到新的剪切板事件 - WM_CLIPBOARDUPDATE\n");
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//监听剪切板
bool TestAddClipboardFormatListener()
{
	WNDCLASSEX wndClass = { sizeof(WNDCLASSEX) };
	wndClass.lpfnWndProc = ClipWndProc;
	wndClass.lpszClassName = L"ClipWnd";
	if (!RegisterClassEx(&wndClass))
	{
		printf("RegisterClassEx error 0x%08X\n", GetLastError()); return 1;
	}
	HWND hWnd = CreateWindowEx(0, wndClass.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
	if (!hWnd)
	{
		printf("CreateWindowEx error 0x%08X\n", GetLastError()); return 2;
	}
	printf("Press ^C to exit\n\n");

	MSG msg;
	while (BOOL bRet = GetMessage(&msg, 0, 0, 0))
	{
		if (bRet == -1)
		{
			printf("GetMessage error 0x%08X\n", GetLastError()); return 3;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return true;
}

// 将文件路径复制到剪贴板
bool CopyFilePathToClipboard(const std::vector<std::wstring>& filePaths) {
	// 打开剪贴板
	if (OpenClipboard(nullptr)) {
		// 清空剪贴板
		EmptyClipboard();

		// 计算文件路径数据的总长度
		size_t totalLength = 0;
		for (const auto& filePath : filePaths) {
			totalLength += filePath.size() + 1; // 加上空字符
		}

		// 在剪贴板上分配内存
		HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, totalLength * sizeof(wchar_t));
		if (hClipboardData != nullptr) {
			// 锁定内存，以便写入数据
			wchar_t* clipboardData = static_cast<wchar_t*>(GlobalLock(hClipboardData));
			if (clipboardData != nullptr) {
				// 将文件路径复制到剪贴板内存中
				wchar_t* currentPos = clipboardData;
				for (const auto& filePath : filePaths) {
					wcscpy_s(currentPos, filePath.size() + 1, filePath.c_str());
					currentPos += filePath.size() + 1;
				}

				// 解锁内存
				GlobalUnlock(hClipboardData);

				// 将内存句柄设置为剪贴板数据
				SetClipboardData(CF_HDROP, hClipboardData);
			}
			else {
				std::cerr << "Failed to lock clipboard memory." << std::endl;
			}
		}
		else {
			std::cerr << "Failed to allocate clipboard memory." << std::endl;
		}

		// 关闭剪贴板
		CloseClipboard();
	}
	else {
		std::cerr << "Failed to open clipboard." << std::endl;
		return false;
	}

	return true;
}

bool SetFilesToClip(std::vector<std::string>& filePaths)
{
	UINT uDropEffect;
	HGLOBAL hGblEffect;
	LPDWORD lpdDropEffect;
	DROPFILES stDrop;

	HGLOBAL hGblFiles;
	LPSTR lpData;

	uDropEffect = RegisterClipboardFormat(TEXT("DropEffect"));
	hGblEffect = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(DWORD));
	if (hGblEffect)
	{
		lpdDropEffect = (LPDWORD)GlobalLock(hGblEffect);
		if (lpdDropEffect)
		{
			*lpdDropEffect = DROPEFFECT_COPY;//复制; 剪贴则用DROPEFFECT_MOVE
		}
		GlobalUnlock(hGblEffect);
	}
	else
	{
		return false;
	}

	stDrop.pFiles = sizeof(DROPFILES);
	stDrop.pt.x = 0;
	stDrop.pt.y = 0;
	stDrop.fNC = FALSE;
	stDrop.fWide = FALSE;

	int filesNameLen = 1;
	for (auto path : filePaths)
	{
		filesNameLen += strlen(path.c_str()) + 1;
	}

	hGblFiles = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(DROPFILES) + filesNameLen);
	if (hGblFiles)
	{
		lpData = (LPSTR)GlobalLock(hGblFiles);
		if (lpData)
		{
			int offset = sizeof(DROPFILES);
			memcpy(lpData, &stDrop, sizeof(DROPFILES));
			for (auto path : filePaths)
			{
				strcpy_s(lpData + offset, strlen(path.c_str()) + 1, path.c_str());
				offset += strlen(path.c_str()) + 1;
			}

		}
		GlobalUnlock(hGblFiles);
	}
	else
	{
		return false;
	}

	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		SetClipboardData(CF_HDROP, hGblFiles);
		SetClipboardData(uDropEffect, hGblEffect);
		CloseClipboard();
		return true;
	}
	else
	{
		return false;
	}
}

std::vector<std::string> GetFilepathFromClip()
{
	std::vector<std::string> path_list;

	if (::OpenClipboard(NULL)) // 打开剪切板
	{
		HDROP hDrop = HDROP(::GetClipboardData(CF_HDROP)); // 获取剪切板中复制的文件列表相关句柄

		if (hDrop != NULL)
		{
			WCHAR szFilePathName[MAX_PATH + 1] = { 0 };

			UINT nNumOfFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); // 得到文件个数

			// 考虑到用户可能同时选中了多个对象(可能既包含文件也包含文件夹)，所以要循环处理
			for (UINT nIndex = 0; nIndex < nNumOfFiles; ++nIndex)
			{
				memset(szFilePathName, 0, MAX_PATH + 1);
				DragQueryFile(hDrop, nIndex, szFilePathName, MAX_PATH);  // 得到文件名

				_bstr_t path(szFilePathName);
				std::string ss = (LPCSTR)path;
				std::wcout << L"读取剪切板中的文件名：" << path << std::endl;

				path_list.push_back(ss);
			}
		}

		::CloseClipboard(); // 关闭剪切板
	}

	return path_list;
}

// 从 IDataObject 接口中获取文件列表
std::vector<std::wstring> GetFileListFromDataObject(IDataObject* pDataObject) {
	std::vector<std::wstring> fileList;

	// 根据 CF_HDROP 格式获取数据
	FORMATETC formatEtc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgMedium;

	if (SUCCEEDED(pDataObject->GetData(&formatEtc, &stgMedium))) {
		// 获取文件列表
		HDROP hDrop = static_cast<HDROP>(GlobalLock(stgMedium.hGlobal));
		if (hDrop != nullptr) {
			UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);

			for (UINT i = 0; i < fileCount; ++i) {
				wchar_t filePath[MAX_PATH];
				if (DragQueryFileW(hDrop, i, filePath, MAX_PATH) > 0) {
					fileList.push_back(filePath);
				}
			}

			GlobalUnlock(stgMedium.hGlobal);
		}

		ReleaseStgMedium(&stgMedium);
	}

	return fileList;
}

int OleGetFilepathFromClip() {
	// 初始化 COM 库
	HRESULT hr = CoInitialize(nullptr);
	if (SUCCEEDED(hr)) {
		IDataObject* pDataObject = nullptr;

		// 获取剪贴板上的 IDataObject 接口
		hr = OleGetClipboard(&pDataObject);
		if (SUCCEEDED(hr)) {
			// 从 IDataObject 接口中获取文件列表
			std::vector<std::wstring> fileList = GetFileListFromDataObject(pDataObject);

			// 输出文件列表
			for (const auto& filePath : fileList) {
				std::wcout << L"Ole读取剪切板中的文件名：" << filePath << std::endl;
			}

			// 释放 IDataObject 接口
			pDataObject->Release();
		}
		else {
			std::cerr << "Failed to get IDataObject from clipboard." << std::endl;
		}

		// 反初始化 COM 库
		CoUninitialize();
	}
	else {
		std::cerr << "Failed to initialize COM." << std::endl;
	}

	return 0;
}

//ole把剪切板中的文件粘贴到指定文件夹
bool PasteFileFromClip(const std::wstring& strPath)
{
	bool bRet = false;
	if (OpenClipboard(NULL))
	{
		HANDLE hClipboardData = GetClipboardData(CF_HDROP);
		if (hClipboardData)
		{
			HDROP hDrop = (HDROP)GlobalLock(hClipboardData);
			if (hDrop)
			{
				UINT nFileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
				if (nFileCount > 0)
				{
					for (UINT i = 0; i < nFileCount; ++i)
					{
						UINT nFilePathLen = DragQueryFile(hDrop, i, NULL, 0);
						if (nFilePathLen > 0)
						{
							std::wstring strFilePath;
							strFilePath.resize(nFilePathLen);
							DragQueryFile(hDrop, i, &strFilePath[0], nFilePathLen + 1);
							std::wstring strFileName = strFilePath.substr(strFilePath.find_last_of(L"\\") + 1);
							std::wstring strNewFilePath = strPath + L"\\" + strFileName;
							if (CopyFile(strFilePath.c_str(), strNewFilePath.c_str(), FALSE))
							{
								bRet = true;
							}
						}
					}
				}
				GlobalUnlock(hDrop);
			}
		}
		CloseClipboard();
	}
	return bRet;
}

// 初始化 FORMATETC 结构
void InitFormatEtc(FORMATETC& formatEtc, CLIPFORMAT cfFormat, TYMED tymed)
{
	formatEtc.cfFormat = cfFormat;
	formatEtc.ptd = nullptr;
	formatEtc.dwAspect = DVASPECT_CONTENT;
	formatEtc.lindex = -1;
	formatEtc.tymed = tymed;
}

// 检查文件夹是否可以粘贴
BOOL CanPasteInFolder()
{
	// 获取特定文件夹的 IShellFolder 接口
	// 这里以桌面为例，实际应用中需要根据需要获取目标文件夹的 IShellFolder 接口
	CComPtr<IShellFolder> pDesktopFolder;
	HRESULT hr = SHGetDesktopFolder(&pDesktopFolder);
	if (FAILED(hr))
	{
		std::cout << "Cannot paste in the desktop folder." << std::endl;
		return FALSE;
	}

	// 如果文件夹没有 drop target，则无法粘贴
	CComPtr<IDropTarget> pDropTarget;
	hr = pDesktopFolder->CreateViewObject(nullptr, IID_IDropTarget, (void**)&pDropTarget);
	if (FAILED(hr))
	{
		std::cout << "Cannot paste in the desktop folder." << std::endl;
		return FALSE;
	}

	// 只有在剪贴板中存在 CFSTR_SHELLIDLIST 格式时才能粘贴
	CComPtr<IDataObject> pDataObj;
	hr = OleGetClipboard(&pDataObj);
	if (FAILED(hr))
	{
		std::cout << "Cannot paste in the desktop folder." << std::endl;
		return FALSE;
	}

	STGMEDIUM medium;
	FORMATETC formatetc;

	// 设置 FORMATETC 结构
	InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
	hr = pDataObj->GetData(&formatetc, &medium);
	if (FAILED(hr))
	{
		std::cout << "Cannot paste in the desktop folder." << std::endl;
		return FALSE;
	}

	// 释放内存
	ReleaseStgMedium(&medium);

	std::cout << "Can paste in the desktop folder." << std::endl;
	return TRUE;
}

// Function to copy files to clipboard using CF_HDROP format
bool CopyFilesToClipboard(const std::vector<std::wstring>& files) {
	if (!OpenClipboard(NULL)) {
		std::cerr << "Failed to open clipboard!" << std::endl;
		return false;
	}

	EmptyClipboard();

	// Calculate the size of the DROPFILES structure and the file paths
	size_t size = sizeof(DROPFILES);
	for (const auto& file : files) {
		size += (file.size() + 1) * sizeof(wchar_t); // File name size + NULL terminator
	}
	size += sizeof(wchar_t); // Additional NULL terminator at the end

	// Allocate memory for the DROPFILES structure
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!hMem) {
		CloseClipboard();
		std::cerr << "Failed to allocate memory!" << std::endl;
		return false;
	}

	// Fill the DROPFILES structure
	DROPFILES* pDropFiles = (DROPFILES*)GlobalLock(hMem);
	pDropFiles->pFiles = sizeof(DROPFILES);
	pDropFiles->fNC = FALSE;
	pDropFiles->fWide = TRUE; // Unicode format

	// Copy the file paths into the allocated memory
	wchar_t* pData = (wchar_t*)((BYTE*)pDropFiles + sizeof(DROPFILES));
	for (const auto& file : files) {
		wcscpy_s(pData, file.size() + 1, file.c_str());
		pData += file.size() + 1; // Move pointer to the next file path
	}
	*pData = L'\0'; // Add the final NULL terminator

	GlobalUnlock(hMem);

	// Set the data in the clipboard
	if (!SetClipboardData(CF_HDROP, hMem)) {
		GlobalFree(hMem);
		CloseClipboard();
		std::cerr << "Failed to set clipboard data!" << std::endl;
		return false;
	}

	CloseClipboard();
	return true;
}

int TestOleGetClipboard_Text()
{
	// 初始化 COM
	HRESULT hr = OleInitialize(nullptr);
	if (FAILED(hr)) {
		std::wcerr << L"Failed to initialize OLE. Error: " << std::hex << hr << std::endl;
		return 1;
	}

	IDataObject* pDataObject = nullptr;
	hr = OleGetClipboard(&pDataObject);
	if (FAILED(hr)) {
		std::wcerr << L"Failed to get clipboard data. Error: " << std::hex << hr << std::endl;
		OleUninitialize();
		return 1;
	}

	FORMATETC fmtetc = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stgmed;

	hr = pDataObject->GetData(&fmtetc, &stgmed);
	if (SUCCEEDED(hr)) {
		// 访问剪贴板数据
		wchar_t* pData = static_cast<wchar_t*>(GlobalLock(stgmed.hGlobal));
		if (pData) {
			std::wcout << L"Clipboard Text: " << pData << std::endl;
			GlobalUnlock(stgmed.hGlobal);
		}
		ReleaseStgMedium(&stgmed);
	}
	else {
		std::wcerr << L"Failed to retrieve clipboard text. Error: " << std::hex << hr << std::endl;
	}

	pDataObject->Release();
	OleUninitialize();
	return 0;
}

// 自定义的 IEnumFORMATETC 实现
class CEnumFormatEtc : public IEnumFORMATETC {
private:
	ULONG m_refCount;
	std::vector<FORMATETC> m_formats;
	ULONG m_index;

public:
	CEnumFormatEtc(const std::vector<FORMATETC>& formats)
		: m_refCount(1), m_formats(formats), m_index(0) {}

	// IUnknown 方法
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
		if (riid == IID_IEnumFORMATETC || riid == IID_IUnknown) {
			*ppv = static_cast<IEnumFORMATETC*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef() override {
		return InterlockedIncrement(&m_refCount);
	}

	ULONG STDMETHODCALLTYPE Release() override {
		ULONG count = InterlockedDecrement(&m_refCount);
		if (count == 0) {
			delete this;
		}
		return count;
	}

	// IEnumFORMATETC 方法
	HRESULT STDMETHODCALLTYPE Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched) override {
		ULONG fetched = 0;
		while (m_index < m_formats.size() && fetched < celt) {
			rgelt[fetched] = m_formats[m_index++];
			fetched++;
		}
		if (pceltFetched) *pceltFetched = fetched;
		return (fetched == celt) ? S_OK : S_FALSE;
	}

	HRESULT STDMETHODCALLTYPE Skip(ULONG celt) override {
		m_index += celt;
		return (m_index < m_formats.size()) ? S_OK : S_FALSE;
	}

	HRESULT STDMETHODCALLTYPE Reset() override {
		m_index = 0;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC** ppenum) override {
		if (!ppenum) return E_POINTER;
		*ppenum = new CEnumFormatEtc(m_formats);
		(*ppenum)->AddRef();
		return S_OK;
	}
};

// 自定义的 IDataObject 实现
class CSimpleDataObject : public IDataObject {
private:
	LONG m_refCount;
	std::wstring m_text;  // 使用std::wstring来支持Unicode

public:
	CSimpleDataObject(const wchar_t* text) : m_refCount(1), m_text(text) {}

	// IUnknown 方法
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
		if (riid == IID_IDataObject || riid == IID_IUnknown) {
			*ppv = static_cast<IDataObject*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef() override {
		return InterlockedIncrement(&m_refCount);
	}

	ULONG STDMETHODCALLTYPE Release() override {
		ULONG count = InterlockedDecrement(&m_refCount);
		if (count == 0) {
			delete this;
		}
		return count;
	}

	// IDataObject 方法
	HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pFormatetc, STGMEDIUM* pMedium) override {
		if (pFormatetc->cfFormat == CF_UNICODETEXT && (pFormatetc->tymed & TYMED_HGLOBAL)) {
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (m_text.size() + 1) * sizeof(wchar_t));
			if (!hMem) return E_OUTOFMEMORY;

			wchar_t* pData = static_cast<wchar_t*>(GlobalLock(hMem));
			wcscpy_s(pData, m_text.size() + 1, m_text.c_str());
			GlobalUnlock(hMem);

			pMedium->tymed = TYMED_HGLOBAL;
			pMedium->hGlobal = hMem;
			pMedium->pUnkForRelease = nullptr;
			return S_OK;
		}
		return DV_E_FORMATETC;
	}

	HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* pFormatetc) override {
		if (pFormatetc->cfFormat == CF_UNICODETEXT && (pFormatetc->tymed & TYMED_HGLOBAL))
			return S_OK;
		return DV_E_FORMATETC;
	}

	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc) override {
		if (dwDirection != DATADIR_GET) return E_NOTIMPL; // 只支持获取数据
		if (!ppenumFormatEtc) return E_POINTER;

		// 定义支持的格式
		std::vector<FORMATETC> formats = {
			{ CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
		};

		*ppenumFormatEtc = new CEnumFormatEtc(formats);
		(*ppenumFormatEtc)->AddRef();
		return S_OK;
	}

	// 其他方法暂不实现
	HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC*, STGMEDIUM*) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC*, FORMATETC*) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE SetData(FORMATETC*, STGMEDIUM*, BOOL) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA**) override { return E_NOTIMPL; }
};

int SetClipboardTextViaOLE() {
	// 初始化OLE库
	if (FAILED(OleInitialize(nullptr))) {
		MessageBoxW(nullptr, L"OleInitialize 失败", L"错误", MB_OK);
		return 1;
	}

	// 创建包含文本的数据对象
	CSimpleDataObject* dataObj = new CSimpleDataObject(L"Hello from OleSetClipboard!");

	// 将数据对象放置到剪贴板
	HRESULT hr = OleSetClipboard(dataObj);
	if (SUCCEEDED(hr)) {
		MessageBoxA(nullptr, "文本已复制到剪贴板", "成功", MB_OK);
		dataObj->Release(); // OleSetClipboard会持有引用，此处释放
	}
	else {
		MessageBoxA(nullptr, "OleSetClipboard 失败", "错误", MB_OK);
		dataObj->Release();
	}

	// 清理OLE库
	OleUninitialize();
	return 0;
}

// 隐藏窗口的类名和标题
const wchar_t g_szClassName[] = L"ClipboardMonitorClass";
HWND g_hwndClipboardMonitor = nullptr;

// 注册剪贴板监听
bool RegisterClipboardListener() {
	if (!AddClipboardFormatListener(g_hwndClipboardMonitor)) {
		std::cerr << "注册剪贴板监听失败！错误代码: " << GetLastError() << std::endl;
		return false;
	}
	return true;
}

// 处理剪贴板内容
void HandleClipboardUpdate() {
	// 初始化 OLE
	if (FAILED(OleInitialize(nullptr))) {
		std::cerr << "OleInitialize 失败！" << std::endl;
		return;
	}

	// 获取剪贴板中的 OLE 数据对象
	IDataObject* pDataObject = nullptr;
	HRESULT hr = OleGetClipboard(&pDataObject);
	if (SUCCEEDED(hr) && pDataObject) {
		// 检查是否包含文本格式
		FORMATETC fmt = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		if (SUCCEEDED(pDataObject->QueryGetData(&fmt))) {
			STGMEDIUM stg;
			if (SUCCEEDED(pDataObject->GetData(&fmt, &stg))) {
				if (stg.tymed == TYMED_HGLOBAL) {
					wchar_t* pText = static_cast<wchar_t*>(GlobalLock(stg.hGlobal));
					if (pText) {
						std::wcout << L"剪贴板内容已更新: " << pText << std::endl;
						GlobalUnlock(stg.hGlobal);
					}
				}
				ReleaseStgMedium(&stg);
			}
		}
		pDataObject->Release();
	}

	OleUninitialize();
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLIPBOARDUPDATE:

		// 调用函数处理剪贴板内容
		HandleClipboardUpdate();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

// 创建隐藏窗口
bool CreateHiddenWindow() {
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = g_szClassName;

	if (!RegisterClassEx(&wc)) {
		std::cerr << "注册窗口类失败！错误代码: " << GetLastError() << std::endl;
		return false;
	}

	g_hwndClipboardMonitor = CreateWindowEx(
		0,
		g_szClassName,
		L"Clipboard Monitor Window",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		100, 100,
		nullptr, nullptr, GetModuleHandle(nullptr), nullptr
	);

	if (!g_hwndClipboardMonitor) {
		std::cerr << "创建窗口失败！错误代码: " << GetLastError() << std::endl;
		return false;
	}

	ShowWindow(g_hwndClipboardMonitor, SW_HIDE); // 隐藏窗口
	return true;
}

int GetClipboardTextViaOLE() {
	// 创建隐藏窗口
	if (!CreateHiddenWindow()) {
		return 1;
	}

	// 注册剪贴板监听
	if (!RegisterClipboardListener()) {
		return 1;
	}

	// 消息循环
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// 清理
	RemoveClipboardFormatListener(g_hwndClipboardMonitor);
	DestroyWindow(g_hwndClipboardMonitor);
	return 0;
}

bool SaveClipboardBitmapToFile(const wchar_t* filename) {
	if (!OpenClipboard(nullptr)) {
		return false;
	}

	HBITMAP hBitmap = nullptr;
	bool success = false;

	// 检查剪贴板中是否有位图数据
	if (IsClipboardFormatAvailable(CF_BITMAP)) {
		hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
	}
	else if (IsClipboardFormatAvailable(CF_DIB)) {
		hBitmap = (HBITMAP)GetClipboardData(CF_DIB);
	}

	if (hBitmap) {
		BITMAP bmp;
		GetObject(hBitmap, sizeof(BITMAP), &bmp);

		// 构造位图文件头和信息头
		BITMAPFILEHEADER bmfHeader{};
		BITMAPINFOHEADER bi{};
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = bmp.bmWidth;
		bi.biHeight = bmp.bmHeight; // 高度为正，表示图像是从底部到顶部存储的
		bi.biPlanes = 1;
		bi.biBitCount = bmp.bmBitsPixel;
		bi.biCompression = BI_RGB;

		DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
		bi.biSizeImage = dwBmpSize;

		bmfHeader.bfType = 0x4D42; // "BM"
		bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bmfHeader.bfSize = bmfHeader.bfOffBits + dwBmpSize;

		// 分配内存存储位图数据
		std::vector<BYTE> pixels(dwBmpSize);
		GetBitmapBits(hBitmap, dwBmpSize, pixels.data());

		// 反转位图数据
		std::vector<BYTE> flippedPixels(dwBmpSize);
		int rowSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4; // 每行字节数
		for (int y = 0; y < bmp.bmHeight; y++) {
			memcpy(&flippedPixels[y * rowSize], &pixels[(bmp.bmHeight - 1 - y) * rowSize], rowSize);
		}

		// 写入文件
		std::ofstream file(filename, std::ios::binary);
		if (file) {
			file.write((char*)&bmfHeader, sizeof(BITMAPFILEHEADER));
			file.write((char*)&bi, sizeof(BITMAPINFOHEADER));
			file.write((char*)flippedPixels.data(), dwBmpSize);
			success = true;
		}
	}
	else
	{
		std::wcout << L"当前剪切板不存在图片" << std::endl;
	}

	CloseClipboard();

	if (success)
		std::wcout << L"读取当前剪切板的图片存到本地D:\\clipboard_image.bmp" << std::endl;

	return success;
}

//获取剪切板格式名称
std::wstring GetClipFormatName(UINT format)
{
	WCHAR strLog[MAX_PATH] = { 0 };
	swprintf_s(strLog, L"0x%X\t", format);
	wstring formatName = strLog;

	switch (format) {
	case CF_TEXT: return formatName + L"CF_TEXT";
	case CF_BITMAP: return formatName + L"CF_BITMAP";
	case CF_METAFILEPICT: return formatName + L"CF_METAFILEPICT";
	case CF_SYLK: return formatName + L"CF_SYLK";
	case CF_DIF: return formatName + L"CF_DIF";
	case CF_TIFF: return formatName + L"CF_TIFF";
	case CF_OEMTEXT: return formatName + L"CF_OEMTEXT";
	case CF_DIB: return formatName + L"CF_DIB";
	case CF_PALETTE: return formatName + L"CF_PALETTE";
	case CF_PENDATA: return formatName + L"CF_PENDATA";
	case CF_RIFF: return formatName + L"CF_RIFF";
	case CF_WAVE: return formatName + L"CF_WAVE";
	case CF_UNICODETEXT: return formatName + L"CF_UNICODETEXT";
	case CF_ENHMETAFILE: return formatName + L"CF_ENHMETAFILE";
	case CF_HDROP: return formatName + L"CF_HDROP";
	case CF_LOCALE: return formatName + L"CF_LOCALE";
	case CF_DIBV5: return formatName + L"CF_DIBV5";
	case CF_OWNERDISPLAY: return formatName + L"CF_OWNERDISPLAY";
	case CF_DSPTEXT: return formatName + L"CF_DSPTEXT";
	case CF_DSPBITMAP: return formatName + L"CF_DSPBITMAP";
	case CF_DSPMETAFILEPICT: return formatName + L"CF_DSPMETAFILEPICT";
	case CF_DSPENHMETAFILE: return formatName + L"CF_DSPENHMETAFILE";
	default:
		// 获取注册的格式名称
		WCHAR buffer[256] = { 0 };
		int length = GetClipboardFormatNameW(format, buffer, ARRAYSIZE(buffer));
		formatName += L"#";
		if (length > 0)
			formatName += buffer;
		else
			formatName += L"未知格式";
		return formatName;
	}
}

std::wstring GetFormatTypeDescription(UINT format) {
	if (format >= 0xC000) {
		return L"动态注册格式\t";
	}
	else if (format >= 0x0200 && format <= 0xFFFF) {
		return L"应用程序私有格式";
	}
	else if (format >= 0x0001 && format <= 0x017F) {
		return L"系统预定义格式";
	}
	return L"特殊系统格式\t";
}

// 获取剪贴板中所有格式
std::vector<std::wstring> GetClipboardFormats() {
	std::vector<std::wstring> formats;

	if (!OpenClipboard(nullptr)) {
		std::wcerr << L"无法打开剪贴板" << std::endl;
		return formats;
	}

	UINT format = 0;
	while ((format = EnumClipboardFormats(format)) != 0) {
		formats.push_back(GetFormatTypeDescription(format) + L"\tFormat:" + GetClipFormatName(format));
	}

	CloseClipboard();
	return formats;
}

// 创建纯红色 200x200 位图
HBITMAP CreateRedBitmap()
{
	const int width = 200;
	const int height = 200;

	// 初始化位图信息头
	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height; // 负数表示从上到下的位图
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;    // 使用32位格式简化对齐
	bmi.bmiHeader.biCompression = BI_RGB;

	// 创建DIB段
	void* pBits = nullptr;
	HBITMAP hBitmap = CreateDIBSection(
		nullptr,
		&bmi,
		DIB_RGB_COLORS,
		&pBits,
		nullptr,
		0
	);

	if (hBitmap && pBits)
	{
		// 填充ARGB数据 (0xFF = 不透明)
		DWORD* pPixels = static_cast<DWORD*>(pBits);
		const DWORD redColor = 0xFFFF0000; // ARGB格式：Alpha=FF, R=FF, G=00, B=00

		for (int i = 0; i < width * height; ++i)
		{
			pPixels[i] = redColor;
		}
	}

	return hBitmap;
}

//将图片写入剪切板
bool BitmapToClipboard(HBITMAP hBM, HWND hWnd)
{
	if (!::OpenClipboard(NULL))
		return false;
	::EmptyClipboard();

	BITMAP bm;
	::GetObject(hBM, sizeof(bm), &bm);

	BITMAPINFOHEADER bi;
	::ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bm.bmWidth;
	bi.biHeight = bm.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = bm.bmBitsPixel;
	bi.biCompression = BI_RGB;
	if (bi.biBitCount <= 1)	// make sure bits per pixel is valid
		bi.biBitCount = 1;
	else if (bi.biBitCount <= 4)
		bi.biBitCount = 4;
	else if (bi.biBitCount <= 8)
		bi.biBitCount = 8;
	else // if greater than 8-bit, force to 24-bit
		bi.biBitCount = 24;

	// Get size of color table.
	SIZE_T dwColTableLen = (bi.biBitCount <= 8) ? (1 << bi.biBitCount) * sizeof(RGBQUAD) : 0;

	// Create a device context with palette
	HDC hDC = ::GetDC(NULL);
	HPALETTE hPal = static_cast<HPALETTE>(::GetStockObject(DEFAULT_PALETTE));
	HPALETTE hOldPal = ::SelectPalette(hDC, hPal, FALSE);
	::RealizePalette(hDC);

	// Use GetDIBits to calculate the image size.
	::GetDIBits(hDC, hBM, 0, static_cast<UINT>(bi.biHeight), NULL,
		reinterpret_cast<LPBITMAPINFO>(&bi), DIB_RGB_COLORS);
	// If the driver did not fill in the biSizeImage field, then compute it.
	// Each scan line of the image is aligned on a DWORD (32bit) boundary.
	if (0 == bi.biSizeImage)
		bi.biSizeImage = ((((bi.biWidth * bi.biBitCount) + 31) & ~31) / 8) * bi.biHeight;

	// Allocate memory
	HGLOBAL hDIB = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPINFOHEADER) + dwColTableLen + bi.biSizeImage);
	if (hDIB)
	{
		union tagHdr_u
		{
			LPVOID             p;
			LPBYTE             pByte;
			LPBITMAPINFOHEADER pHdr;
			LPBITMAPINFO       pInfo;
		} Hdr;

		Hdr.p = ::GlobalLock(hDIB);
		// Copy the header
		::CopyMemory(Hdr.p, &bi, sizeof(BITMAPINFOHEADER));
		// Convert/copy the image bits and create the color table
		int nConv = ::GetDIBits(hDC, hBM, 0, static_cast<UINT>(bi.biHeight),
			Hdr.pByte + sizeof(BITMAPINFOHEADER) + dwColTableLen,
			Hdr.pInfo, DIB_RGB_COLORS);
		::GlobalUnlock(hDIB);
		if (!nConv)
		{
			::GlobalFree(hDIB);
			hDIB = NULL;
		}
	}
	if (hDIB)
		::SetClipboardData(CF_DIB, hDIB);
	::CloseClipboard();
	::SelectPalette(hDC, hOldPal, FALSE);
	::ReleaseDC(NULL, hDC);
	return NULL != hDIB;
}

// 自定义数据结构保存需要延迟渲染的内容
struct DelayedData {
	std::string textData;
	std::vector<BYTE> bitmapData;
} g_DelayedContent;

// 生成示例位图数据（24bpp 100x100蓝色位图）
void GenerateBitmapData(std::vector<BYTE>& data) {
	const int width = 100;
	const int height = 100;

	BITMAPINFOHEADER bih = { 0 };
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = width;
	bih.biHeight = height;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;

	// 计算行字节数并对齐到4字节
	int stride = (width * 3 + 3) & ~3;
	data.resize(sizeof(BITMAPINFOHEADER) + stride * height);

	// 填充位图头
	memcpy(data.data(), &bih, sizeof(bih));

	// 填充蓝色像素数据
	BYTE* pixels = data.data() + sizeof(bih);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			pixels[x * 3] = 255;     // Blue
			pixels[x * 3 + 1] = 0;   // Green
			pixels[x * 3 + 2] = 0;    // Red
		}
		pixels += stride;
	}
}

// 窗口过程
LRESULT CALLBACK MyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_COMMAND: {
		// 用户点击复制按钮时的处理
		if (LOWORD(wParam) == 1) {
			// 准备延迟渲染数据
			g_DelayedContent.textData = "Delayed Render Example";
			GenerateBitmapData(g_DelayedContent.bitmapData);

			// 打开剪贴板并声明延迟渲染格式
			if (OpenClipboard(hWnd)) {
				EmptyClipboard();
				SetClipboardData(CF_TEXT, NULL);    // 声明文本格式
				SetClipboardData(CF_DIB, NULL);     // 声明位图格式
				CloseClipboard();
			}
		}
		break;
	}
	case WM_RENDERFORMAT: {
		// 系统请求特定格式数据
		UINT format = (UINT)wParam;

		if (format == CF_TEXT) {
			// 创建全局内存对象
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, g_DelayedContent.textData.size() + 1);
			if (hMem) {
				char* pData = static_cast<char*>(GlobalLock(hMem));
				strcpy_s(pData, g_DelayedContent.textData.size() + 1, g_DelayedContent.textData.c_str());
				GlobalUnlock(hMem);
				SetClipboardData(CF_TEXT, hMem);
			}
		}
		else if (format == CF_DIB) {
			if (!g_DelayedContent.bitmapData.empty()) {
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, g_DelayedContent.bitmapData.size());
				if (hMem) {
					BYTE* pData = static_cast<BYTE*>(GlobalLock(hMem));
					memcpy(pData, g_DelayedContent.bitmapData.data(), g_DelayedContent.bitmapData.size());
					GlobalUnlock(hMem);
					SetClipboardData(CF_DIB, hMem);
				}
			}
		}
		break;
	}
	case WM_RENDERALLFORMATS: {
		// 程序即将关闭时渲染所有格式
		OpenClipboard(hWnd);
		EmptyClipboard();

		// 渲染文本
		if (!g_DelayedContent.textData.empty()) {
			HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE, g_DelayedContent.textData.size() + 1);
			char* pText = static_cast<char*>(GlobalLock(hText));
			strcpy_s(pText, g_DelayedContent.textData.size() + 1, g_DelayedContent.textData.c_str());
			GlobalUnlock(hText);
			SetClipboardData(CF_TEXT, hText);
		}

		// 渲染位图
		if (!g_DelayedContent.bitmapData.empty()) {
			HGLOBAL hBmp = GlobalAlloc(GMEM_MOVEABLE, g_DelayedContent.bitmapData.size());
			BYTE* pBmp = static_cast<BYTE*>(GlobalLock(hBmp));
			memcpy(pBmp, g_DelayedContent.bitmapData.data(), g_DelayedContent.bitmapData.size());
			GlobalUnlock(hBmp);
			SetClipboardData(CF_DIB, hBmp);
		}

		CloseClipboard();
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 注册窗口类
int RegisterWindowClass(HINSTANCE hInstance) {
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = MyWndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = L"ClipDemoClass";
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	return RegisterClassEx(&wcex);
}

// 创建窗口
HWND CreateMainWindow(HINSTANCE hInstance) {
	return CreateWindow(
		L"ClipDemoClass",
		L"Clipboard Demo",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		300, 200,
		nullptr, nullptr,
		hInstance, nullptr
	);
}

//设置延迟剪切板
int SetDelayClipboard()
{
	RegisterWindowClass(GetModuleHandle(nullptr));
	HWND hWnd = CreateMainWindow(GetModuleHandle(nullptr));

	// 创建示例按钮
	CreateWindow(L"BUTTON", L"Copy", WS_VISIBLE | WS_CHILD,
		10, 10, 80, 25, hWnd, (HMENU)1, GetModuleHandle(nullptr), nullptr);

	ShowWindow(hWnd, SW_SHOWNA);
	UpdateWindow(hWnd);

	// 消息循环
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

int main()
{
	// 设置本地化
	std::locale::global(std::locale(""));

	//复制粘贴文件
	if (0)
	{
		std::vector<std::string> filePaths = { "E:\\Users\\Enlink\\Desktop\\111.txt", "E:\\Users\\Enlink\\Desktop\\222.txt" };
		SetFilesToClip(filePaths);

		//打印剪切板中的文件路径
		GetFilepathFromClip();
		OleGetFilepathFromClip();

		//粘贴到指定文件夹
		PasteFileFromClip(L"E:\\Users\\Enlink\\Desktop\\test");

		//判断桌面是否可以粘贴
		CanPasteInFolder();

		SendMessageW((HWND)0x0147211A, WM_CLIPBOARDUPDATE, 0, 0);
		getchar();
		return 0;
	}

	if (1)
	{
		bool bFirst = true;
		int iInput = 0;
		while (1)
		{
			if (!bFirst)
				std::wcout << std::endl;
			bFirst = false;

			std::wcout << L"输入1 = 枚举当前剪切板类型" << std::endl;
			std::wcout << L"输入2 = 将文字设置到剪切板\t\t输入3 = 读取当前剪切板的文字" << std::endl;
			std::wcout << L"输入4 = 将图片设置到剪切板\t\t输入5 = 读取当前剪切板的图片" << std::endl;
			std::wcout << L"输入6 = 清理剪切板" << std::endl;
			std::wcout << L"输入7 = 监听剪切板" << std::endl;
			std::wcout << L"输入8 = 设置延迟剪切板" << std::endl;
			std::wcout << L"输入10 = OleSetClipboard\t\t输入11 = OleGetClipboard" << std::endl;
			std::wcout << L"当前窗口工作站的剪贴板序列号:" << GetClipboardSequenceNumber()<< std::endl;
			std::wcin >> iInput;

			if (iInput == 10)
			{
				SetClipboardTextViaOLE();
			}
			else if (iInput == 11)
			{
				std::wcout << L"开始监听剪切板..." << std::endl;
				GetClipboardTextViaOLE();
				return 0;
			}
			else if (iInput == 1)
			{
				//枚举当前剪切板类型
				std::vector<std::wstring> formats = GetClipboardFormats();
				if (formats.empty()) {
					std::wcout << L"剪贴板为空或未找到支持的格式" << std::endl;
				}
				else {
					std::wcout << L"剪贴板中的格式总共【"<< CountClipboardFormats() << L"】个，格式如下：" << std::endl;
					for (const auto& format : formats) {
						std::wcout << L"- " << format << std::endl;
					}
				}
			}
			else if (iInput == 2)
			{
				//将文字设置到剪切板
				SetClipboardText(L"TestClip - SetTextFromClip 测试程序");
			}
			else if (iInput == 3)
			{
				//剪切板读取文字
				GetTextFromClip();
			}
			else if (iInput == 4)
			{
				//剪切板设置图片 生成 200x200 红色图片
				HBITMAP hRedBmp = CreateRedBitmap();
				if (hRedBmp)
				{
					// 设置到剪切板
					BitmapToClipboard(hRedBmp, NULL);
					DeleteObject(hRedBmp);
					std::cout << "红色图片已写入剪切板\n";
				}
			}
			else if (iInput == 5)
			{
				//剪切板读取图片
				SaveClipboardBitmapToFile(L"D:\\clipboard_image.bmp");
			}
			else if (iInput == 6)
			{
				std::wcout << L"清理剪切板" << std::endl;
				ClearClip();
			}
			else if (iInput == 7)
			{
				//监听剪切板
				TestAddClipboardFormatListener();
			}
			else if (iInput == 8)
			{
				//设置延迟剪切板
				SetDelayClipboard();
			}
		}
	}

	return 0;
}