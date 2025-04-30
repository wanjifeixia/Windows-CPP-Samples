#include <windows.h>
#include <stdio.h>
#include <string>
#include <gdiplus.h>
#include <winspool.h>
#include <iostream>

#pragma comment(lib, "gdiplus.lib")

//截图水印
void DrawWatermark(int width, int height, void* pData, HDC hDC = NULL)
{
	RECT rc;
	rc.top = 100;
	rc.left = 100;
	rc.bottom = 300;
	rc.right = 300;

	HBRUSH greenBrush = CreateSolidBrush(RGB(0, 255, 0));
	FillRect(hDC, &rc, greenBrush);
	DeleteObject(greenBrush);
	
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	Gdiplus::Graphics graphics(hDC);
	Gdiplus::FontFamily fontFamily(L"Arial");
	Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

	Gdiplus::SolidBrush solidBrush(Gdiplus::Color(255, 255, 0, 100));
	std::wstring text(L"Hello, world!\r\n撒大声地\r\n嘎嘎嘎");

	for (int i = 50; i < width; i += 200)
	{
		for (int k = 50; k < height; k += 150)
		{
			//旋转绘图区域
			Gdiplus::Matrix matrix;
			matrix.RotateAt(-45, Gdiplus::PointF(i, k));
			graphics.SetTransform(&matrix);

			graphics.DrawString(text.c_str(), -1, &font, Gdiplus::PointF(i, k), &solidBrush);
		}
	}
}

//这个函数用于读取bmp图像文件，用于给打印机打印的时候使用
//info是位图信息结构
//file是文件名
//dib_ptr是位图rgb像素数据指针，输出用的，所以请提供一个void**
bool read_bmp(BITMAPINFO& info, const char* file, void** dib_ptr)
{
	BITMAPFILEHEADER file_handle;
	FILE* f = NULL;
	fopen_s(&f, file, "rb");
	if (f == 0) return false;

	memset(&info, 0, sizeof(info));
	fread(&file_handle, 1, sizeof(file_handle), f);
	fread(&info.bmiHeader, 1, sizeof(info.bmiHeader), f);
	fseek(f, file_handle.bfOffBits, SEEK_SET);
	void* dib = malloc(info.bmiHeader.biSizeImage);
	fread(dib, 1, info.bmiHeader.biSizeImage, f);
	fclose(f);
	*dib_ptr = dib;
	return true;
}

//释放位图的数据指针，释放空间
void release_bmp(void** dib_ptr)
{
	if (dib_ptr != 0)
	{
		if (*dib_ptr != 0)
		{
			free(*dib_ptr);
			*dib_ptr = 0;
		}
	}
}

int TestEnumPrinters()
{
	DWORD numPrinters = 0;
	DWORD bufferSize = 0;

	// 获取打印机数量
	EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &bufferSize, &numPrinters);

	// 分配足够的内存来存储打印机信息
	BYTE* printerInfoBuffer = new BYTE[bufferSize];
	PRINTER_INFO_2A* printerInfo = reinterpret_cast<PRINTER_INFO_2A*>(printerInfoBuffer);

	// 枚举打印机
	if (EnumPrintersA(PRINTER_ENUM_LOCAL, NULL, 2, printerInfoBuffer, bufferSize, &bufferSize, &numPrinters))
	{
		std::cout << "打印机数量：" << numPrinters << std::endl;

		// 遍历打印机信息并输出
		for (DWORD i = 0; i < numPrinters; i++)
		{
			std::cout << "打印机名称：" << printerInfo[i].pPrinterName << std::endl;
			std::cout << "打印机驱动程序：" << printerInfo[i].pDriverName << std::endl;
			std::cout << "打印机端口：" << printerInfo[i].pPortName << std::endl;
			std::cout << std::endl;
		}
	}
	else
	{
		std::cout << "无法枚举打印机。错误码：" << GetLastError() << std::endl;
	}

	// 释放内存
	delete[] printerInfoBuffer;

	return 0;
}

int TestPrinterBmp()
{
	PRINTDLG printInfo = { 0 };
	printInfo.lStructSize = sizeof(printInfo);
	printInfo.Flags = PD_RETURNDC | PD_RETURNDEFAULT | PD_ALLPAGES;

	//PD_RETURNDEFAULT 意味着直接返回当前系统默认的打印机设置，若没有这个标识，则会弹出对话框让用户自己选择
	//PD_RETURNDC 意味着返回的是dc而不是ic（information context）
	//PD_ALLPAGES 指定“全部”单选钮在初始时被选中(缺省标志)
	//对于错误的时候，若需要知道更多的错误消息，请执行CommDlgError来查看返回值

	//PrintDlg目的是获取当前系统设置的默认打印机相关信息，供后面使用
	if (!PrintDlg(&printInfo))
	{
		//清除标志，然后执行将会弹出对话框让用户选择打印机
		printInfo.Flags = 0;
		if (!PrintDlg(&printInfo))
		{
			printf("没有选择打印机。\n");
			return 0;
		}
	}

	//获取打印的时候的dc，然后往这个dc上绘图就是打印出来的样子了
	HDC hPrintDC = printInfo.hDC;

	//锁定全局对象，获取对象指针。 devmode是有关设备初始化和打印机环境的信息
	DEVMODE* devMode = (DEVMODE*)GlobalLock(printInfo.hDevMode);
	if (devMode == 0)
	{
		printf("获取打印机设置时发生了错误.\n");
		return 0;
	}

	devMode->dmPaperSize = DMPAPER_A4;				//打印机纸张设置为A4。
	devMode->dmOrientation = DMORIENT_PORTRAIT;		//打印方向设置成纵向打印
	//DMORIENT_LANDSCAPE 是横向打印
	//对打印方向的设置，会影响hPrintDC的大小，假设宽度为1024，高度为300
	//则在横向打印的时候dc大小会是宽1024 * 高300
	//而纵向打印的时候dc大小会是宽300 * 高1024

	int printQuality = devMode->dmPrintQuality;			//获取打印机的打印质量
	devMode->dmColor = DMCOLOR_COLOR;
	devMode->dmSpecVersion = 1025;
	devMode->dmDriverVersion = 1539;
	devMode->dmSize = 220;
	devMode->dmDriverExtra = 5200;

	//设置打印质量的，因为像素被打印到纸上的时候是有做转换的
	//单位是dpi，意为像素每英寸(dots per inch)。就是一英寸的纸张上
	//打印多少个像素点，意味着这个质量越高，打印结果越精细，越低，越粗糙
	//设置的质量可以是具体数值，也可以是宏DMRES_MEDIUM
	//一般我们选择300，或者600，DMRES_MEDIUM = 600dpi

	//应用我们修改过后的设置.
	ResetDC(hPrintDC, devMode);

	//解锁全局对象，对应GlobalLock
	GlobalUnlock(printInfo.hDevMode);

	//设置绘图模式，以保证绘图可以不失真的绘制上去，因为StretchDIBits函数要求设置这个才能够不是失真的绘图
	//当你用StretchDIBits绘图往窗口显示的时候就会发现，24位图，若没有这个设置，是会失真的
	SetStretchBltMode(hPrintDC, HALFTONE);

	//读取位图，待会画在hPrintDC上面去
	BITMAPINFO bmp_info;
	int image_width = 0, image_height = 0;
	void* dib_ptr = 0;
	if (!read_bmp(bmp_info, "ceshi.bmp", &dib_ptr))
	{
		printf("读取位图miku.bmp失败了.\n");
		return 0;
	}
	image_width = bmp_info.bmiHeader.biWidth;
	image_height = bmp_info.bmiHeader.biHeight;
	printf("位图大小：%d x %d\n", image_width, image_height);

	//设置打印时候的字体
	LOGFONT lf;
	lf.lfHeight = -printQuality * 1 / 2.54;
	//打印出来的字像素高度有n个，注意是像素高度，打印到纸上的时候是需要将
	//像素转换成实际尺寸单位，比如你需要在纸上打印高度为1cm的字，当你打印质量为600dpi的时候
	//这里就设置为 -236, -600dpi * 1cm / 2.54 = -236pix
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = 5;		//这里设置字体重量，意味着字体的厚度
	lf.lfItalic = false;	//斜体
	lf.lfUnderline = false;	//下划线
	lf.lfStrikeOut = 0;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = 0;
	lf.lfClipPrecision = 0;
	lf.lfQuality = PROOF_QUALITY;
	lf.lfPitchAndFamily = 0;
	wcscpy_s(lf.lfFaceName, L"宋体");	//使用宋体进行打印

	//实际上这一步并不是必须的，因为默认打印机设置已经配置好了，这里只是我们自己固定好字体
	HFONT hUseFont = CreateFontIndirect(&lf);					//创建字体
	HFONT hOldFont = (HFONT)SelectObject(hPrintDC, hUseFont);	//选用创建的字体

	//获取dc的大小，实际上还有一种HORZRES和VERTRES就是宽度和高度，但是我查得到的结果说计算下来准确的
	//HORZSIZE 是Horizontal size in millimeters，页面宽度（水平），单位毫米mm
	//VERTSIZE 是Vertical size in millimeters，页面高度（垂直），单位毫米mm
	//LOGPIXELSX 是Logical pixels/inch in X，x方向的逻辑像素每英寸.单位 pix / inch，像素每英寸
	//LOGPIXELSY 是Logical pixels/inch in Y，y方向的逻辑像素每英寸.单位 pix / inch，像素每英寸
	//不用管逻辑是个什么东西，不理会他，知道单位是pix / inch就行了
	//1 inch = 2.54 cm，所以这里是 mm / 25.4 * pix / inch，得到的结果就是dc大小像素数为单位
	int dc_page_width = GetDeviceCaps(hPrintDC, HORZSIZE) / 25.4 * GetDeviceCaps(hPrintDC, LOGPIXELSX);
	int dc_page_height = GetDeviceCaps(hPrintDC, VERTSIZE) / 25.4 * GetDeviceCaps(hPrintDC, LOGPIXELSY);

	//好了，可以开始打印了
	DOCINFO doc_info = { sizeof(DOCINFO), L"测试打印机" };
	//cbSize
	//结构体的字节大小
	//lpszDocName
	//指明文档名的字符串指针，该字符串以null为尾。
	//lpszOutput
	//指明输出文件的名称的字符串指针，该字符串以null为尾。如果指针为null，那么输出将会发送至某个设备，该设备将由传递至 StartDoc 函数的‘设备上下文句柄’HDC来指明。
	//lpszDatatype
	//指针指向代表某种数据类型的字符串，而数据用于记录打印工作，该字符串以null为尾。合法值可通过调用函数 EnumPrintProcessorDatatypes 可得到，亦可为 NULL。
	//fwType
	//指明打印工作的其它信息。可为0或者以下值之一：
	//DI_APPBANDING
	//DI_ROPS_READ_DESTINATION

	//开始一个档案，打印的时候是按照档案来区分的，返回作业编号（大于0的为正常）
	int doc_idd = StartDoc(hPrintDC, &doc_info);
	if (doc_idd <= 0)
	{
		printf("StartDoc 错误，错误代码：%d\n", GetLastError());
		return -1;
	}
	printf("作业编号：%d\n", doc_idd);

	XFORM xform;
	BOOL bResult = GetWorldTransform(hPrintDC, &xform);

	//开始一个打印页面，大于等于0为正确
	if (StartPage(hPrintDC) < 0)
	{
		printf("StartPage 错误\n");
		AbortDoc(hPrintDC);	//结束doc打印
		return -1;
	}

	//设定文字和图像绘制的区域
	RECT rcText = { 0, 30, dc_page_width, 300 };
	RECT rcImage = { 30, 300, dc_page_width - 30, dc_page_height - 30 };

	//写下文字
	DrawText(hPrintDC, L"miku，测试打印图片", -1, &rcText, DT_VCENTER | DT_SINGLELINE | DT_CENTER);

	//将图片绘制到dc上，给打印机打印出来
	//这个函数带拉伸功能的，若dest的宽度高度和src不一致的时候他会拉伸
	StretchDIBits(hPrintDC, rcImage.left, rcImage.top, rcImage.right - rcImage.left,
		rcImage.bottom - rcImage.top, 0, 0, image_width, image_height,
		dib_ptr, &bmp_info, DIB_RGB_COLORS, SRCCOPY);

	xform.eM11 = 2;
	xform.eM22 = 2;
	SetGraphicsMode(hPrintDC, GM_ADVANCED);   //一定要首先打开GM_ADVANCED。
	SetWorldTransform(hPrintDC, &xform);
	//GetWorldTransform(hPrintDC, &xform);
	//ModifyWorldTransform(hPrintDC, &xform, MWT_LEFTMULTIPLY);
	DrawWatermark(GetDeviceCaps(hPrintDC, HORZRES), GetDeviceCaps(hPrintDC, VERTRES), NULL, hPrintDC);

	//结束这一页，其实可以循环的startpage、endpage，这样在一个文档中进行多个页面的打印
	EndPage(hPrintDC);


	//开始一个打印页面，大于等于0为正确
	if (StartPage(hPrintDC) < 0)
	{
		printf("StartPage 错误\n");
		AbortDoc(hPrintDC);	//结束doc打印
		return -1;
	}

	//写下文字
	DrawText(hPrintDC, L"miku，测试打印图片", -1, &rcText, DT_VCENTER | DT_SINGLELINE | DT_CENTER);

	//将图片绘制到dc上，给打印机打印出来
	//这个函数带拉伸功能的，若dest的宽度高度和src不一致的时候他会拉伸
	StretchDIBits(hPrintDC, rcImage.left, rcImage.top, rcImage.right - rcImage.left,
		rcImage.bottom - rcImage.top, 0, 0, image_width, image_height,
		dib_ptr, &bmp_info, DIB_RGB_COLORS, SRCCOPY);

	xform.eM11 = 1;
	xform.eM22 = 1;
	SetWorldTransform(hPrintDC, &xform);
	//ModifyWorldTransform(hPrintDC, &xform, MWT_LEFTMULTIPLY);
	DrawWatermark(GetDeviceCaps(hPrintDC, HORZRES), GetDeviceCaps(hPrintDC, VERTRES), NULL, hPrintDC);

	//结束这一页，其实可以循环的startpage、endpage，这样在一个文档中进行多个页面的打印
	EndPage(hPrintDC);

	EndDoc(hPrintDC);
	printf("打印顺利完成了. o(∩_∩)o \n");

last_code:
	//选取旧的字体
	SelectObject(hPrintDC, hOldFont);

	//删除gdi对象，释放内存
	DeleteObject(hUseFont);

	//释放位图内存
	release_bmp(&dib_ptr);
	return 0;
}

int main()
{
	//设置本地化，让控制台输出中文
	std::locale::global(std::locale(""));
	std::wcout.imbue(std::locale(""));

	bool bFirst = true;
	int iInput = 0;
	while (1)
	{
		if (!bFirst)
			std::wcout << std::endl;
		bFirst = false;

		std::wcout << L"输入1 = 枚举打印机" << std::endl;
		std::wcout << L"输入2 = 使用打印机打印文字+图片: ceshi.bmp" << std::endl;
		std::wcin >> iInput;

		if (iInput == 1)
		{
			TestEnumPrinters();
		}
		else if (iInput == 2)
		{
			TestPrinterBmp();
		}
	}
	return 0;
}