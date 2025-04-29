// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include "comdef.h"
#include <sstream>
#include <assert.h>
#include "wincodec.h"
#include "D3DCommon.h"
#include "D3D11.h"
#include "DXGI.h"
#include "DXGI1_2.h"
#include <fstream>
#include <iostream>
#include <ctime>
#include <chrono>
#include <atlcomcli.h>
#include <ShlObj.h>
using std::ofstream;
using namespace std::chrono;

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "d3d11.lib")

const int FRAME_RATE_30 = 30;

long long getTS(){
	return duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
}

//水印
void DrawWatermark(int width, int height, int stride, void* pData)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	Gdiplus::Bitmap bmp(width, height, stride, PixelFormat32bppARGB, (BYTE*)pData);
	if (bmp.GetLastStatus() == Gdiplus::Ok)
	{
		Gdiplus::Graphics graphics(&bmp);
		Gdiplus::FontFamily fontFamily(L"Arial");
		Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

		Gdiplus::SolidBrush solidBrush(Gdiplus::Color(100, 255, 0, 100));
		std::wstring text(L"Hello, world!\r\n撒大声地\r\n嘎嘎嘎");

		for (int i = 50; i < width; i += 200)
		{
			for (int k = 50; k < height; k += 150)
			{
				Gdiplus::Matrix matrix;
				matrix.RotateAt(-45, Gdiplus::PointF(i, k));
				graphics.SetTransform(&matrix);

				graphics.DrawString(text.c_str(), -1, &font, Gdiplus::PointF(i, k), &solidBrush);
			}
		}
	}
}

static size_t WINAPI HookVtbl(void* pObject, size_t classIdx, size_t methodIdx, size_t newMethod)
{
	size_t** vtbl = (size_t**)pObject;
	DWORD oldProtect = 0;
	size_t oldMethod = vtbl[classIdx][methodIdx];
	VirtualProtect(vtbl[classIdx] + sizeof(size_t*) * methodIdx, sizeof(size_t*), PAGE_READWRITE, &oldProtect);
	vtbl[classIdx][methodIdx] = newMethod;
	VirtualProtect(vtbl[classIdx] + sizeof(size_t*) * methodIdx, sizeof(size_t*), oldProtect, &oldProtect);
	return oldMethod;
}

static size_t WINAPI GetHookVtbl(void* pObject, size_t classIdx, size_t methodIdx, size_t newMethod)
{
	size_t** vtbl = (size_t**)pObject;
	DWORD oldProtect = 0;
	size_t oldMethod = vtbl[classIdx][0];
	WCHAR strLog[MAX_PATH] = { 0 };
	swprintf_s(strLog, L"New_D3D11CreateDevice vtbl:%p oldMethod:%p newMethod:%p", *vtbl, oldMethod, newMethod);
	OutputDebugStringW(strLog);
	return oldMethod;
}

typedef HRESULT (STDMETHODCALLTYPE* IDXGIOutputDuplication_AcquireNextFrame)(
	IDXGIOutputDuplication* This,
	/* [annotation][in] */
	_In_  UINT TimeoutInMilliseconds,
	/* [annotation][out] */
	_Out_  DXGI_OUTDUPL_FRAME_INFO* pFrameInfo,
	/* [annotation][out] */
	_COM_Outptr_  IDXGIResource** ppDesktopResource);

IDXGIOutputDuplication_AcquireNextFrame Old_IDXGIOutputDuplication_AcquireNextFrame = NULL;

HRESULT STDMETHODCALLTYPE New_IDXGIOutputDuplication_AcquireNextFrame(
	IDXGIOutputDuplication* This,
	/* [annotation][in] */
	_In_  UINT TimeoutInMilliseconds,
	/* [annotation][out] */
	_Out_  DXGI_OUTDUPL_FRAME_INFO* pFrameInfo,
	/* [annotation][out] */
	_COM_Outptr_  IDXGIResource** ppDesktopResource)
{
	HRESULT hr = Old_IDXGIOutputDuplication_AcquireNextFrame(This, TimeoutInMilliseconds, pFrameInfo, ppDesktopResource);

	CComPtr<ID3D11Texture2D> texture;
	(*ppDesktopResource)->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&texture);
	assert((*ppDesktopResource) != nullptr);

	CComPtr<ID3D11Device> pDevice;
	texture->GetDevice(&pDevice);
	CComPtr<ID3D11DeviceContext> pD3D11DeviceContext;
	pDevice->GetImmediateContext(&pD3D11DeviceContext);

	CComPtr<ID3D11Texture2D> textureBuf;
	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_TEXTURE2D_DESC tempDesc;

	if (texture != nullptr) {
		texture->GetDesc(&tempDesc);

		ZeroMemory(&textureDesc, sizeof(textureDesc));

		textureDesc.MiscFlags = 0;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.BindFlags = 0;
		textureDesc.Width = tempDesc.Width;
		textureDesc.Height = tempDesc.Height;
		textureDesc.Format = tempDesc.Format;
		textureDesc.ArraySize = tempDesc.ArraySize;
		textureDesc.SampleDesc = tempDesc.SampleDesc;
		textureDesc.Usage = D3D11_USAGE_STAGING;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

		//textureDesc.SampleDesc.Count = 1;
		//textureDesc.SampleDesc.Quality = 0;
		//textureDesc.Format = DXGI_FORMAT_NV12; // NV12

		pDevice->CreateTexture2D(&textureDesc, NULL, &textureBuf);

		CComPtr<ID3D11DeviceContext> pD3D11DeviceContext;
		pDevice->GetImmediateContext(&pD3D11DeviceContext);
		pD3D11DeviceContext->CopyResource(textureBuf, texture);

		//方案一
		if (1)
		{
			D3D11_MAPPED_SUBRESOURCE  mapResource;
			pD3D11DeviceContext->Map(textureBuf, 0, D3D11_MAP_READ_WRITE, NULL, &mapResource);
			DrawWatermark(tempDesc.Width, tempDesc.Height, mapResource.RowPitch, mapResource.pData);
			pD3D11DeviceContext->Unmap(textureBuf, 0);
		}

		//方案二
		if (0)
		{
			IDXGISurface* hStagingSurf = NULL;
			textureBuf->QueryInterface(__uuidof(IDXGISurface), (void**)(&hStagingSurf));
			DXGI_MAPPED_RECT mappedRect;
			hStagingSurf->Map(&mappedRect, DXGI_MAP_READ);
			DrawWatermark(tempDesc.Width, tempDesc.Height, mappedRect.Pitch, mappedRect.pBits);
			hStagingSurf->Unmap();
		}

		//(*ppDesktopResource)->Release();
		//*ppDesktopResource = (IDXGIResource*)textureBuf;
		pD3D11DeviceContext->CopyResource(texture, textureBuf);
	}
	return hr;
}

#include <vector>
#include "DXGIScreenCapture.h"
void dxgi_v2()
{
	DXGIScreenCapture sc;
	sc.Init();
	Sleep(200);
	sc.CaptureImage("img_dxgi2.bmp");
}

#include "image_capture.h"
void dxgi_v3()
{
	DXGI_InitCapture();
	Capture* a = DXGI_GetCapture();
}

int main(int argc, char* argv[])
{
	//dxgi_v2();
	//dxgi_v3();
	//return 1;
	ID3D11Device* d3ddevice = nullptr;
	ID3D11DeviceContext* d3dcontext = nullptr;
	D3D_FEATURE_LEVEL feature_level;
	D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG || D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0, D3D11_SDK_VERSION, &d3ddevice, &feature_level, &d3dcontext);
	IDXGIDevice* device = nullptr;
	d3ddevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&device);
	IDXGIAdapter* adapter = nullptr;
	device->GetParent(__uuidof(IDXGIAdapter), (void**)&adapter);
	/*
	IDXGIFactory1* factory = nullptr;
	adapter->GetParent(__uuidof(IDXGIFactory1), (void**)&factory);
	IDXGIAdapter1* adapter1 = nullptr;
	factory->EnumAdapters1(0, &adapter1);
	*/
	IDXGIOutput* output = nullptr;
	for (int i = 0;; i++)
	{
		if (adapter->EnumOutputs(i, &output) == DXGI_ERROR_NOT_FOUND)
		{
			std::cout << "No output detected." << std::endl;
			return -1;
		}
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);
		if (desc.AttachedToDesktop) break;
	}
	IDXGIOutput1* output1 = nullptr;
	output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
	IUnknown* unknown = d3ddevice;
	IDXGIOutputDuplication* output_duplication = nullptr;
	_com_error err(output1->DuplicateOutput(unknown, &output_duplication));
	std::wcout << err.ErrorMessage() << __LINE__ << std::endl;

	Old_IDXGIOutputDuplication_AcquireNextFrame = (IDXGIOutputDuplication_AcquireNextFrame)HookVtbl(output_duplication, 0, 8, (size_t)New_IDXGIOutputDuplication_AcquireNextFrame);

	int size = 0;
	byte *bytes = new byte[size];

	int index = 0, cur_index = 0;
	HRESULT hr;

	long long ts1 = 0, ts2 = 0;
	int targetFramInterval = 1000 / (FRAME_RATE_30 + 0.5);

	if (argc > 1) {
		std::cout << "target frame rate: " << argv[1] << std::endl;
		float targetFrameRate = atof(argv[1]);
		if (targetFrameRate > 0)
			targetFramInterval = 1000 / (targetFrameRate + 0.5);
	}

	bool save_img = true;
	if (argc > 2){
		save_img = true;
		std::cout << "save image enabled: " << std::endl;
		system("mkdir img_dxgi");
		system("del img_dxgi\\*.argb");
	}

	float elapsedTime = 0, cur_elapsedTime = 0;
	while (true)
	{
		ts1 = getTS();

		if (cur_index > 0){
			float cur_frameRate = cur_index * 1000 / cur_elapsedTime;
			float avg_frameRate = index * 1000 / elapsedTime;
			std::cout << "\r" << "avg_frameRate: " << avg_frameRate << ", cur_frameRate: " << cur_frameRate << std::flush;
		}

		DXGI_OUTDUPL_FRAME_INFO frame_info = { 0 };
		IDXGIResource* resource = nullptr;
		while (frame_info.AccumulatedFrames == 0)
		{

			frame_info = { 0 };
			if (resource != nullptr)
			{
				resource->Release();
				output_duplication->ReleaseFrame();
			}
			resource = nullptr;
			err = _com_error(output_duplication->AcquireNextFrame(INFINITE, &frame_info, &resource));
			if (err.Error() == DXGI_ERROR_ACCESS_LOST)
			{
				output_duplication->Release();
				output1->DuplicateOutput(unknown, &output_duplication);
			}
		}

		ID3D11Texture2D* texture = nullptr;
		resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&texture);
		assert(resource != nullptr);

		ID3D11Texture2D* textureBuf;
		D3D11_TEXTURE2D_DESC textureDesc;
		//ComPtr<ID3D11Texture2D> tempTexture = this->c_PreviewTexture;
		D3D11_TEXTURE2D_DESC tempDesc;

		if (texture != nullptr) {
			texture->GetDesc(&tempDesc);

			//std::wcout << "width: " << tempDesc.Width << ", height: " << tempDesc.Height << ", org_fmt: " << tempDesc.Format << ", mip_lvl: " << tempDesc.MipLevels << std::endl;

			ZeroMemory(&textureDesc, sizeof(textureDesc));

			textureDesc.MiscFlags = 0;
			textureDesc.MipLevels = 1;
			textureDesc.ArraySize = 1;
			textureDesc.BindFlags = 0;
			textureDesc.Width = tempDesc.Width;
			textureDesc.Height = tempDesc.Height;
			textureDesc.Format = tempDesc.Format;
			textureDesc.ArraySize = tempDesc.ArraySize;
			textureDesc.SampleDesc = tempDesc.SampleDesc;
			textureDesc.Usage = D3D11_USAGE_STAGING;
			textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

			//textureDesc.SampleDesc.Count = 1;
			//textureDesc.SampleDesc.Quality = 0;
			//textureDesc.Format = DXGI_FORMAT_NV12; // NV12

			d3ddevice->CreateTexture2D(&textureDesc, NULL, &textureBuf);
			d3dcontext->CopyResource(textureBuf, texture);
			D3D11_MAPPED_SUBRESOURCE  mapResource;

			//方案一
			hr = d3dcontext->Map(textureBuf, 0, D3D11_MAP_READ, NULL, &mapResource);

			//方案二
// 			IDXGISurface* hStagingSurf = NULL;
// 			textureBuf->QueryInterface(__uuidof(IDXGISurface), (void**)(&hStagingSurf));
// 			DXGI_MAPPED_RECT mappedRect;
// 			hr = hStagingSurf->Map(&mappedRect, DXGI_MAP_READ);

			/*IDXGISurface* CopySurface = nullptr;
			hr = textureBuf->QueryInterface(__uuidof(IDXGISurface), (void **)&CopySurface);
			DXGI_MAPPED_RECT mapResource;
			hr = CopySurface->Map(&mapResource, DXGI_MAP_READ);*/

			if (FAILED(hr)) {
				std::wcout << "d3dcontext->Map error: " << hr << std::endl;
			}
			else {

				int tsize = mapResource.DepthPitch;
				if (tsize != size) {
					size = tsize;
					if (bytes != nullptr)
						delete[] bytes;
					bytes = new byte[size];
				}

				//std::wcout << "size: " << size << ", ts: " << getTS() << std::endl;

				memcpy(bytes, mapResource.pData, size);
				//mapResource = { 0 };

				if (save_img)
				{
					char exePath[MAX_PATH] = { 0 };
					GetCurrentDirectoryA(MAX_PATH, exePath);
					std::cout << "d3d截图:" << exePath << "\\img_dxgi.bmp" << std::endl;

					char file[100] = "";
					sprintf_s(file, "img_dxgi.bmp");
					ofstream fout;
					fout.open(file, std::ios::binary);

					// construct bitmap
					BITMAPINFOHEADER bmif;
					bmif.biSize = sizeof(BITMAPINFOHEADER);
					bmif.biHeight = tempDesc.Height;
					bmif.biWidth = tempDesc.Width;
					bmif.biSizeImage = bmif.biWidth * bmif.biHeight * 4;
					bmif.biPlanes = 1;
					bmif.biBitCount = (WORD)(bmif.biSizeImage / bmif.biHeight / bmif.biWidth * 8);
					bmif.biCompression = BI_RGB;

					BITMAPFILEHEADER bmfh;
					LONG offBits = sizeof(BITMAPFILEHEADER) + bmif.biSize;
					bmfh.bfType = 0x4d42; // "BM"
					bmfh.bfOffBits = offBits;
					bmfh.bfSize = offBits + bmif.biSizeImage;
					bmfh.bfReserved1 = 0;
					bmfh.bfReserved2 = 0;

					fout.write((const char*)&bmfh, sizeof(BITMAPFILEHEADER));
					fout.write((const char*)&bmif, sizeof(BITMAPINFOHEADER));

					//错误写法
					//fout.write((char*)mapResource.pData, size);
					
					//正确写法
// 					for (uint32_t y = 0; y < bmif.biHeight; y++) {
// 						fout.write((char*)mapResource.pData + y * mapResource.RowPitch, bmif.biWidth * 4);
// 					}
					for (int h = bmif.biHeight - 1; h >= 0; h--) {
						fout.write((char*)mapResource.pData + h * mapResource.RowPitch, bmif.biWidth * 4);
					}
					fout.close();
				}
			}
			//CopySurface->Release();
			textureBuf->Release();
		}

		texture->Release();
		resource->Release();
		output_duplication->ReleaseFrame();
		index += 1;
		cur_index += 1;
		ts2 = getTS();
		int sleep_duration = ts2 - ts1;
		sleep_duration = sleep_duration > targetFramInterval ? 0 : targetFramInterval - sleep_duration;
		Sleep(sleep_duration);

		elapsedTime += getTS() - ts1;
		cur_elapsedTime += getTS() - ts1;
		if (cur_elapsedTime > 1000){
			cur_elapsedTime = 0;
			cur_index = 0;
		}
		break;
	}

	return 0;
}
