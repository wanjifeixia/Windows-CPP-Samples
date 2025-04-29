#include "WindowHelper.h"

namespace DX {

std::vector<Monitor> GetMonitors()
{
	std::vector<Monitor> monitors;

	HRESULT hr = S_OK;

	IDirect3D9Ex* d3d9ex = nullptr;
	hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
	if (FAILED(hr)) {
		return monitors;
	}

	int adapter_count = d3d9ex->GetAdapterCount();

	for (int i = 0; i < adapter_count; i++) {
		Monitor monitor;
		memset(&monitor, 0, sizeof(Monitor));

		LUID luid = { 0 , 0 };
		hr = d3d9ex->GetAdapterLUID(i, &luid);
		if (FAILED(hr)) {
			continue;
		}

		monitor.low_part = (uint64_t)luid.LowPart;
		monitor.high_part = (uint64_t)luid.HighPart;

		HMONITOR hMonitor = d3d9ex->GetAdapterMonitor(i);
		if (hMonitor) {
			MONITORINFO monitor_info;
			monitor_info.cbSize = sizeof(MONITORINFO);
			BOOL ret = GetMonitorInfoA(hMonitor, &monitor_info);
			if (ret) {
				monitor.left = monitor_info.rcMonitor.left;
				monitor.right = monitor_info.rcMonitor.right;
				monitor.top = monitor_info.rcMonitor.top;
				monitor.bottom = monitor_info.rcMonitor.bottom;
				monitors.push_back(monitor);
			}
		}
	}

	d3d9ex->Release();
	return monitors;
}

std::vector <IDXGIAdapter*> EnumerateAdapters(void)
{
    IDXGIAdapter * pAdapter = nullptr;
    std::vector <IDXGIAdapter*> vAdapters;
    IDXGIFactory* pFactory = nullptr;


    // Create a DXGIFactory object.
    if(FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory) ,(void**)&pFactory)))
    {
        return vAdapters;
    }


    for ( UINT i = 0;
         pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND;
         ++i )
    {
        vAdapters.push_back(pAdapter);
    }


    if(pFactory)
    {
        pFactory->Release();
    }

    return vAdapters;

}

}
