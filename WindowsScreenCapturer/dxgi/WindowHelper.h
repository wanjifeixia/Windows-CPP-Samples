#ifndef WINDOW_HELPER_H
#define WINDOW_HELPER_H

#include <vector>
#include <d3d9.h>
#include <dxgi.h>
namespace DX {

struct Monitor
{
	uint64_t low_part;
	uint64_t high_part;

	int left;
	int top;
	int right;
	int bottom;
};

std::vector<Monitor> GetMonitors();//d3d9接口获取
std::vector <IDXGIAdapter*> EnumerateAdapters(void);//dxgi接口获取
}

#endif
