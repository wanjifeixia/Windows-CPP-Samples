// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <ctime>
#include <chrono>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace std::chrono;
using namespace std;
using std::ofstream;

const float FRAME_RATE_30 = 30;

typedef struct {
	HWND window;
	HDC windowDC;
	HDC memoryDC;
	HBITMAP bitmap;
	BITMAPINFOHEADER bitmapInfo;

	int width;
	int height;

	void *pixels;
} grabber_t;

grabber_t *grabber_create(HWND window, HDC hdc = NULL);
void grabber_destroy(grabber_t *self);
void *grabber_grab(grabber_t *self);

long long getTS(){
	return duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
}

wstring Format(LPCWSTR pszFormat, ...)
{
	wstring ret;
	va_list ptr;
	va_start(ptr, pszFormat);
	int nlen = _vscwprintf(pszFormat, ptr);
	if (nlen > 0)
	{
		ret.resize(nlen + 1);
		nlen = vswprintf_s((wchar_t*)ret.data(), nlen + 1, pszFormat, ptr);
		ret.resize(nlen);
	}
	va_end(ptr);
	return std::move(ret);
}

int main(int argc, char* argv[])
{
	//³£¹æ½ØÍ¼
	HDC hDcGetDesktopWindow = GetDC(NULL);
	HWND hWnd1 = WindowFromDC(hDcGetDesktopWindow);
	int cx1 = GetDeviceCaps(hDcGetDesktopWindow, HORZRES);
	int cy1 = GetDeviceCaps(hDcGetDesktopWindow, VERTRES);

	//ÓÐµÀÔÆ±Ê¼Ç½ØÍ¼
	POINT pt;
	GetCursorPos(&pt);
	HDC hDcMonitor = NULL;
	HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	if (hMonitor) {
		MONITORINFOEXW info;
		info.cbSize = sizeof(info);
		if (GetMonitorInfoW(hMonitor, &info)) {
			hDcMonitor = CreateDCW(info.szDevice, NULL, NULL, NULL);
			if (hDcMonitor)
			{
				HWND hWnd2 = WindowFromDC(hDcMonitor);
				int cx2 = GetDeviceCaps(hDcMonitor, HORZRES);
				int cy2 = GetDeviceCaps(hDcMonitor, VERTRES);
			}
		}
	}

	int size = 0;
	byte* bytes = new byte[size];

	int index = 0, cur_index = 0;

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
		system("mkdir img_gdi");
		system("del img_gdi\\*.argb");
	}

	char exePath[MAX_PATH] = { 0 };
	GetCurrentDirectoryA(MAX_PATH, exePath);

	grabber_t* source = NULL;
	float elapsedTime = 0, cur_elapsedTime = 0;
	if (1)
	{
		std::cout << "gdi½ØÍ¼:" << exePath << "\\img_gdi.bmp" << std::endl;
		source = grabber_create(GetDesktopWindow());
	}
	else
	{
		std::cout << "gdiÓÐµÀ·­Òë½ØÍ¼:" << exePath << "\\img_gdi.bmp" << std::endl;
		source = grabber_create(GetDesktopWindow(), hDcMonitor);
	}

	while (true)
	{
		ts1 = getTS();

		if (cur_index > 0){
			float cur_frameRate = cur_index * 1000 / cur_elapsedTime;
			float avg_frameRate = index * 1000 / elapsedTime;
			std::cout << "\r" << "avg_frameRate: " << avg_frameRate << ", cur_frameRate: " << cur_frameRate << std::flush;
		}

		
		int tsize = source->bitmapInfo.biSizeImage;
		if (tsize != size) {
			size = tsize;
			if (bytes != nullptr)
				delete[] bytes;
			bytes = new byte[size];
		}

		//std::wcout << "size: " << size << ", ts: " << getTS().count() << std::endl;

		//memcpy(bytes, grabber_grab(source), size);


		if (save_img){
			char file[100] = "";
			sprintf_s(file, "img_gdi.bmp", index);
			ofstream fout;
			fout.open(file, std::ios::binary);

			// construct bitmap
			BITMAPINFOHEADER bmif;
			bmif.biSize = sizeof(BITMAPINFOHEADER);
			bmif.biHeight = source->height;
			bmif.biWidth = source->width;
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
			// construct bitmap
			
			fout.write((char*)grabber_grab(source), size);
			fout.close();
		}

		index += 1;
		cur_index += 1;
		ts2 = getTS();
		int sleep_duration = ts2 - ts1;
		sleep_duration = sleep_duration > targetFramInterval ? 0 : targetFramInterval - sleep_duration;
		//std::cout << "sleeping for: " << sleep_duration << std::endl;
		Sleep(sleep_duration);

		elapsedTime += getTS() - ts1;
		cur_elapsedTime += getTS() - ts1;
		if (cur_elapsedTime > 1000){
			cur_elapsedTime = 0;
			cur_index = 0;
		}
		break;
	}
	grabber_destroy(source);
	return 0;
}

grabber_t *grabber_create(HWND window, HDC hdc/* = NULL*/) {
	grabber_t *self = (grabber_t *)malloc(sizeof(grabber_t));
	memset(self, 0, sizeof(grabber_t));

	if (hdc == NULL)
	{
		RECT rect;
		GetClientRect(window, &rect);
		self->window = window;
		self->width = rect.right - rect.left;
		self->height = rect.bottom - rect.top;
		self->windowDC = GetDC(self->window);
	}
	else
	{
		self->window = WindowFromDC(hdc);
		self->width = GetDeviceCaps(hdc, HORZRES);
		self->height = GetDeviceCaps(hdc, VERTRES);
		self->windowDC = hdc;
	}
	
	self->memoryDC = CreateCompatibleDC(self->windowDC);
	self->bitmap = CreateCompatibleBitmap(self->windowDC, self->width, self->height);
	SetStretchBltMode(self->windowDC, HALFTONE);
	self->bitmapInfo.biSize = sizeof(BITMAPINFOHEADER);
	self->bitmapInfo.biPlanes = 1;
	self->bitmapInfo.biBitCount = 32;
	self->bitmapInfo.biWidth = self->width;
	self->bitmapInfo.biHeight = -self->height;
	self->bitmapInfo.biCompression = BI_RGB;
	self->bitmapInfo.biSizeImage = self->width * self->height * 4;

	self->pixels = malloc(self->bitmapInfo.biSizeImage);
	return self;
}

void grabber_destroy(grabber_t *self) {
	if (self == NULL) { return; }
	ReleaseDC(self->window, self->windowDC);
	DeleteDC(self->memoryDC);
	DeleteObject(self->bitmap);
	free(self->pixels);
	free(self);
}

void *grabber_grab(grabber_t *self) {
	if (self == NULL) { return NULL; }
	// uint32_t t1 = rtc::Time();
	// uint32_t t2;
	// t2 = rtc::Time();
	// LOG(LS_WARNING) << "t1: " << t2 - t1;
	// t1 = t2;
	HGDIOBJ hbmOld = SelectObject(self->memoryDC, self->bitmap);
	BitBlt(self->memoryDC, 0, 0, self->width, self->height, self->windowDC, 0, 0, SRCCOPY | CAPTUREBLT);
	//StretchBlt(self->memoryDC, 0, 0, self->width, self->height, self->windowDC, 0, 0, self->width, self->height, SRCCOPY | CAPTUREBLT);

	GetDIBits(self->memoryDC, self->bitmap, 0, self->height, self->pixels, (BITMAPINFO*)&(self->bitmapInfo), DIB_RGB_COLORS);
	SelectObject(self->memoryDC, hbmOld);
	return self->pixels;
}