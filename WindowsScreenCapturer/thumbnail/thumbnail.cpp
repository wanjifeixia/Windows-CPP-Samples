#include "thumbnail_window.h"

int main()
{
	ThumbnailWindow tw;
	RECT rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = 500;
	rc.bottom = 500;
	tw.Create(rc);

	HWND hSrcWnd = FindWindow(NULL, L"CFF Explorer VIII");

	//禁用AeroPeek就无法截屏了
	BOOL bExcludedFromPeek = TRUE;
	//DwmSetWindowAttribute(hSrcWnd, DWMWA_DISALLOW_PEEK, &bExcludedFromPeek, sizeof(BOOL));
	//DwmSetWindowAttribute(hSrcWnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &bExcludedFromPeek, sizeof(BOOL));

	tw.SetTarget(hSrcWnd);
}